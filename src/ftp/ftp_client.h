#pragma once

#include <string>
#include <vector>

#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

struct FtpConfig {
	std::string url;
	std::string username;
	std::string password;
	bool verbose = false;
};

class FtpClient {
public:
	FtpClient() = default;

	FtpClient(const FtpClient&) = delete;

	FtpClient(FtpClient&& other) noexcept {
		*this = std::move(other);
	}

	~FtpClient() {
		if (!curl) return;
		curl_easy_cleanup(static_cast<CURL*>(curl));
		curl = nullptr;
	}

	FtpClient&
	operator=(const FtpClient&) = delete;

	FtpClient&
	operator=(FtpClient&& other) noexcept {
		if (this == &other) return *this;
		if (curl) curl_easy_cleanup(static_cast<CURL*>(curl));

		curl      = other.curl;
		cfg       = std::move(other.cfg);
		connected = other.connected;

		other.curl = nullptr;
		other.connected = false;

		return *this;
	}

	void
	connect(const FtpConfig& cfg);

	void
	download_to_file(	const std::string& symbol,
						const std::string& name,
						const std::filesystem::path& local_path) const;

	std::vector<std::string>
	list_files(const std::string& symbol) const;

private:
	FtpConfig cfg{};
	void* curl = nullptr;
	bool connected = false;

	void
	reset_state() const;
};
