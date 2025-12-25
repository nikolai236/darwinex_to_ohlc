#pragma once
#include <istream>
#include <ostream>
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


void
decompress_gzip(std::istream& in,
				std::ostream& out,
				spdlog::logger& logger);