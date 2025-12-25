#include <iostream>
#include <ostream>
#include <istream>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <zlib.h>
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "./decompress.h"

namespace {

int
inflate_chunk(	z_stream& stream,
				std::ostream& out,
				std::vector<unsigned char>& out_buf)
{
	stream.next_out = out_buf.data();
	stream.avail_out = static_cast<uInt>(out_buf.size());

	int ret = inflate(&stream, Z_NO_FLUSH);
	if (ret < 0) return ret;

	size_t produced = out_buf.size() - stream.avail_out;
	if (produced) {
		out.write(reinterpret_cast<char*>(out_buf.data()), produced);
	}

	return ret;
}

}

void
decompress_gzip(std::istream& in,
				std::ostream& out,
				spdlog::logger& logger)
{
	z_stream stream{};
	if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
		logger.error("inflateInit2 failed");
		return;
	}

	// 1 MB of data at a time
	std::vector<unsigned char>  in_buf(1 << 20);
	std::vector<unsigned char> out_buf(1 << 20);

	int ret = Z_OK;
	size_t total_in  = 0;
	size_t total_out = 0;

	while (ret != Z_STREAM_END) {
		in.read((char*)in_buf.data(), in_buf.size());

		auto got = in.gcount();
		if (got <= 0) break;

		total_in += static_cast<size_t>(got);

		stream.next_in  = in_buf.data();
		stream.avail_in = static_cast<uInt>(got);

		while (stream.avail_in > 0 && ret != Z_STREAM_END) {
			size_t before = stream.total_out;
 
			ret = inflate_chunk(stream, out, out_buf);
			if (ret < 0) break;

			total_out += (stream.total_out - before);
		}
	}

	inflateEnd(&stream);

	if (ret != Z_STREAM_END) {
		logger.error(
			"gunzip stream ended prematurely (in={} bytes)",
			total_in
		);
		return;
	}

	logger.info(
		"gzip decompression finished ({} → {} bytes)",
		total_in,
		total_out
	);
}