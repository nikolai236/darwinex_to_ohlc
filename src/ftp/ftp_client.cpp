#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "./ftp_client.h"

namespace {

size_t
write_to_string(void* ptr, size_t size, size_t nmemb, void* user_data) {
	auto* out = static_cast<std::string*>(user_data);
	out->append(static_cast<char*>(ptr), size * nmemb);
	return size * nmemb;
}

size_t
write_file_cb(char* ptr, size_t size, size_t nmemb, void* user_data) {
	auto f = static_cast<std::FILE*>(user_data);
	return std::fwrite(ptr, size, nmemb, f);
}

std::string
join_url(const std::string& base, const std::string& rest) {
	if (base.empty()) return rest;
	if (rest.empty()) return base;

	std::string temp_rest = rest;
	if (rest.front() == '/') temp_rest = temp_rest.substr(1);
	if (base.back()  == '/') return base + temp_rest;
	if (rest.back()  != '/') temp_rest += "/";

	return base + "/" + temp_rest;
}

void
throw_curl(CURLcode code, const std::string& what) {
	if (code == CURLE_OK) return;
	throw std::runtime_error(
		std::string(what) + ": " + curl_easy_strerror(code)
	);
}

}

void
FtpClient::connect(const FtpConfig& config) {
	cfg = config;

	if (!curl) {
		curl = curl_easy_init();
		if (!curl) throw std::runtime_error("curl_easy_init failed");
	}

	CURL* h = static_cast<CURL*>(curl);

	throw_curl(curl_easy_setopt(h, CURLOPT_USERNAME, cfg.username.c_str()), "set username");
	throw_curl(curl_easy_setopt(h, CURLOPT_PASSWORD, cfg.password.c_str()), "set password");

	throw_curl(curl_easy_setopt(h, CURLOPT_VERBOSE, cfg.verbose ? 1L : 0L), "set verbose");
	connected = true;
}

void
FtpClient::reset_state() const {
	if (!curl || !connected) return;

	CURL* h = static_cast<CURL*>(curl);

	curl_easy_setopt(h, CURLOPT_URL, nullptr);
	curl_easy_setopt(h, CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, nullptr);
}

void
FtpClient::download_to_file(	const std::string& symbol,
								const std::string& name,
								const std::filesystem::path& local_folder) const
{
	if (!curl || !connected) {
		throw std::runtime_error(name + " FtpClient::connect must be called before download_to_file");\
	}

	reset_state();

	const auto url = join_url(join_url(cfg.url, symbol), name);
	const auto local_file_path = local_folder / name;

	std::FILE* f = std::fopen(local_file_path.string().c_str(), "wb");
	if (!f) throw std::runtime_error("failed to open output file: " + local_folder.string());

	CURL* h = static_cast<CURL*>(curl);
	CURLcode code = CURLE_OK;
	try {
		throw_curl(curl_easy_setopt(h, CURLOPT_URL, url.c_str()), name + " set url");
		throw_curl(curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_file_cb), name + " set writefunction");
		throw_curl(curl_easy_setopt(h, CURLOPT_WRITEDATA, f), name + " set writedata");

		code = curl_easy_perform(h);
		throw_curl(code, name + " curl_easy_perform");

		long inf = 0;
		curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &inf);
		if (inf >= 400) {
			throw std::runtime_error("FTP HTTP-like response code indicates failure: " + std::to_string(code) + " " + name);
		}

		std::fclose(f);
	} catch(...) {
		std::fclose(f);
		throw;
	}
}

std::vector<std::string>
FtpClient::list_files(const std::string& symbol) const {
	if (!curl || !connected) {
		throw std::runtime_error("FtpClient::connect must be called before download_to_file");\
	}

	reset_state();
	CURL* h = static_cast<CURL*>(curl);

	auto url = join_url(cfg.url, symbol);
	std::cout << url << " k\n";

	std::string listing;
	std::vector<std::string> files {};

	throw_curl(curl_easy_setopt(h, CURLOPT_URL, url.c_str()), "set url");
	throw_curl(curl_easy_setopt(h, CURLOPT_DIRLISTONLY, 1L), "set dirlistonly");
    throw_curl(curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_to_string), "set writefunction");
	throw_curl(curl_easy_setopt(h, CURLOPT_WRITEDATA, &listing), "set writedata");

	auto code = curl_easy_perform(h);
	curl_easy_setopt(h, CURLOPT_DIRLISTONLY, 0L);

	throw_curl(code, "curl_easy_perform");
	std::istringstream iss(listing);
	std::string line;

	while (std::getline(iss, line)) {
		if (line.empty()) continue;
		if (line == "." || line == "..") continue;
		files.push_back(line);
	}

	return files;
}
