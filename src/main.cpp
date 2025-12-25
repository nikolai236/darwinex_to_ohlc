#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>
#include <memory>

#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "./writer/writer.h"
#include "./transform/transform.h"
#include "./organizer/organizer.h"
#include "./ftp/ftp_client.h"
#include "./dotenv/dotenv.h"
#include "./decompress/decompress.h"

namespace fs = std::filesystem;

using LogAction = std::function<void(spdlog::logger&)>;

struct CurlGlobal {
    CurlGlobal()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

bool
with_logger(const fs::path& root,
			const std::string& category,
			const std::string& symbol,
			const LogAction& action)
{
	const auto dir = root / category;
	fs::create_directories(dir);

	const auto file = dir / symbol;
	const auto name = category + "_" + symbol;

	auto logger = spdlog::get(name);
	if (!logger) {
		logger = spdlog::basic_logger_mt(
			name, file.string(), false
		);
	}

	try {
		action(*logger);
	} catch (const std::exception& e) {
		logger->error(e.what());
		logger->flush();

		return false;
    }

	return true;
}

int main() {
    load_dotenv(".env");

	const fs::path log_path = std::getenv("LOGS_FOLDER_PATH");
	fs::create_directory(log_path);

	const FtpConfig cfg {
		.url = std::getenv("FTP_URL"),
		.username = std::getenv("FTP_USERNAME"),
		.password = std::getenv("FTP_PASSWORD"),
		.verbose = true,
	};

	CurlGlobal curl_guard;
	const std::string symbol = "ADAUSD";

	const fs::path download_folder = std::getenv("FTP_DOWNLOAD_FOLDER");
	const auto download_symbol_dir = download_folder / symbol;

	with_logger(log_path, "ftp", symbol, [&](spdlog::logger& logger) {

		FtpClient client;
		client.connect(cfg);

		auto remote_files = client.list_files(symbol);
		logger.info(
			"Downloading batch: symbol={}, count={}",
			symbol,
			remote_files.size()
		);

		fs::create_directories(download_symbol_dir);

		for (const auto& filename : remote_files) {
			logger.info("Downloading {}", filename);
			client.download_to_file(symbol, filename, download_symbol_dir);
		}
	});

	const fs::path unzipped_dir = std::getenv("DECOMPRESSED_FOLDER"); 
	fs::create_directories(unzipped_dir);

	with_logger(log_path, "decompress", symbol, [&](spdlog::logger& logger) {
		for (const auto& entry : fs::directory_iterator(download_symbol_dir)) {
			auto gz_path = entry.path();

			std::ifstream in(gz_path, std::ios::binary);
			if (!in) throw std::runtime_error("failed to open input: " + gz_path.string());

			auto out_path = unzipped_dir / gz_path.stem().string();

			std::ofstream out(out_path, std::ios::binary);
			if (!out) throw std::runtime_error("failed to open output: " + out_path.string());

			decompress_gzip(in, out, logger);
		}
	});

	fs::path db_path = std::getenv("DB_PATH");
	BatchOrganizer organizer{unzipped_dir};

	with_logger(log_path, "write", symbol, [&](spdlog::logger& logger) {
		for (const auto& entry : fs::directory_iterator(download_symbol_dir)) {

			const auto name = entry.path().filename().string();
			auto [a, b] = organizer.get_batch(name);

			MultiFileReader ask(a, unzipped_dir);
			MultiFileReader bid(b, unzipped_dir);

			AskBidReader reader { std::move(ask), std::move(bid) };

			Candle c;
			std::vector<Candle> candles{};

			while(reader.get_next_candle(c)) candles.push_back(c);
			write_candles_to_db(candles, db_path, symbol);
		}
	});
}