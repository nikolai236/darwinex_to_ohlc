#pragma once

#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>

#include "../organizer/organizer.h"

namespace fs = std::filesystem;

struct Candle {
	std::int64_t time;
	std::int64_t tick_count;

	double open;
	double high;
	double low;
	double close;
};

struct TickEntry {
	std::int64_t epoch;

	double price;
	double size;
};

bool
get_tick_entry(MultiFileReader& in, TickEntry& out);

class AskBidMerger {
public:
	AskBidMerger() = delete;
	AskBidMerger(MultiFileReader&& ask_stream, MultiFileReader&& bid_stream, std::int64_t frame):
		ask_stream(std::move(ask_stream)), bid_stream(std::move(bid_stream)), frame(frame)
	{
		init();
	}

	bool
	get_next_candle(Candle& out);

private:
	static constexpr std::int64_t GAP_RESET = 1000 * 60; // reset on 1m gaps

	MultiFileReader ask_stream;
	MultiFileReader bid_stream;

	std::int64_t frame;

	bool has_curr_ask = false;
	TickEntry curr_ask{};

	bool has_curr_bid = false;
	TickEntry curr_bid{};

	bool has_last_ask = false;
	TickEntry last_ask{};

	bool has_last_bid = false;
	TickEntry last_bid{};

	bool has_buffered = false;
	TickEntry buffered{};

	void
	init() {
		has_curr_ask = get_tick_entry(ask_stream, curr_ask);
		has_curr_bid = get_tick_entry(bid_stream, curr_bid);
	}

	bool
	get_next_mid_tick(TickEntry& out);

	std::int64_t
	get_last_epoch() const;
};

	// AskBidReader(std::ifstream& ask_stream, std::ifstream& bid_stream):
	// 	ask_stream(std::move(ask_stream)), bid_stream(std::move(bid_stream))
	// {
	// 	if (!ask_stream || !bid_stream) throw std::runtime_error("Bad input stream");
	// 	init();
	// }

	// AskBidReader(const fs::path& ask_path, const fs::path& bid_path):
	// 	ask_stream(ask_path), bid_stream(bid_path)
	// {
	// 	if (!ask_stream) throw std::runtime_error("Failed to open file: " + ask_path.string());
	// 	if (!bid_stream) throw std::runtime_error("Failed to open file: " + bid_path.string());

	// 	init();
	// }