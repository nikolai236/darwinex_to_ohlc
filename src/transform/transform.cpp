#include <cmath>

#include "transform.h"

bool
get_tick_entry(MultiFileReader& in, TickEntry& out) {
	std::string line;

	if (!in.getline(line)) return false;
	if (line.empty()) return false;

	std::istringstream iss(line);
	char comma;
	iss >> out.epoch >> comma >> out.price >> comma >> out.size;

	if (!iss) {
		throw std::runtime_error("Failed to parse tick line: " + line);
	}

	return true;
}

bool
AskBidMerger::get_next_mid_tick(TickEntry& out) {
	while (true) {
		if (!has_curr_ask && !has_curr_bid) return false;
		bool use_bid;

		if (!has_curr_bid) use_bid = false;
		else if (!has_curr_ask) use_bid = true;
		else use_bid = (curr_bid.epoch <= curr_ask.epoch);

		std::int64_t epoch;
		auto last_epoch = get_last_epoch();

		if (use_bid) {
			last_bid = curr_bid;
			has_last_bid = true;

			epoch = last_bid.epoch;
			has_curr_bid = get_tick_entry(bid_stream, curr_bid);
		} else {
			last_ask = curr_ask;
			has_last_ask = true;

			epoch = last_ask.epoch;
			has_curr_ask = get_tick_entry(ask_stream, curr_ask);
		}

		if (last_epoch != -1 && epoch - last_epoch > GAP_RESET) {
			has_last_ask = false;
			has_last_bid = false;
		}

		if (!has_last_ask || !has_last_bid) continue;

		out.epoch = epoch;
		out.price = 0.5 * (last_ask.price + last_bid.price);
		out.size = 1.0;

		return true;
	}

	return false;
}

// nodejs debug check
// require('fs').readFileSync(filePath, 'utf8').split('\n').map(s => s.split(',').map(r => Number(r))).filter(k => k.length >= 5).every(([x, o, h, l, c]) => h >= o && h >= c && l <= o && l <= c)

bool
AskBidMerger::get_next_candle(Candle& out) {
	TickEntry tick{};

	if (has_buffered) {

		tick = buffered;
		has_buffered = false;

	} else if (!get_next_mid_tick(tick)) {
		return false;
	}

	auto bucket = tick.epoch / frame;
	auto open_time = bucket * frame;

	Candle candle {
		.open  = tick.price,
		.close = tick.price,
		.high  = tick.price,
		.low   = tick.price,
		
		.time = open_time,
		.tick_count = 1,
	};

	while (true) {
		TickEntry tick{};
		if (!get_next_mid_tick(tick)) {
			out = candle;
			return true;
		}

		auto curr_bucket = tick.epoch / frame;
		if (curr_bucket != bucket) {

			buffered = tick;
			has_buffered = true;
			break;
		}

		candle.high = std::max(candle.high, tick.price);
		candle.low  = std::min(candle.low, tick.price);

		candle.close = tick.price;
		++candle.tick_count;
	}

	out = candle;
	return true;
}

std::int64_t
AskBidMerger::get_last_epoch() const {
	if (has_last_ask && has_last_bid) {
		return std::max(last_ask.epoch, last_bid.epoch);
	}
	return -1;
}

