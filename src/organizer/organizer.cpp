#include <algorithm>
#include <iostream>

#include "organizer.h"

bool
BatchOrganizer::is_ask(const std::string& filename) const {
	auto pos = filename.find("_ASK_");
	return pos != std::string::npos;
}

bool
BatchOrganizer::is_bid(const std::string& filename) const {
	auto pos = filename.find("_BID_");
	return pos != std::string::npos;
}

std::string
BatchOrganizer::get_key(const std::string& filename) const {
	auto first  = filename.find('_');
	auto second = filename.find('_', first + 1);
	auto third  = filename.find('_', second + 1);

	return filename.substr(second + 1, third - (second + 1));
}

int
BatchOrganizer::extract_hour(const std::string& filename) const {
	auto pos = filename.rfind('_');
	// if (pos == std::string::npos) return -1;
	std::string hour_part = filename.substr(pos + 1);

	auto dot = hour_part.find('.');
	hour_part = hour_part.substr(0, dot);

	return std::stoi(hour_part);
}

void
BatchOrganizer::sort_by_hour(std::vector<std::string>& filenames) const {
	auto fn = [&](const std::string& a, const std::string& b) {
		return extract_hour(a) < extract_hour(b);
	};
	std::sort(filenames.begin(), filenames.end(), fn);
}

void
BatchOrganizer::populate_dict(const fs::path& directory) {
	for (const auto& entry : fs::directory_iterator(directory)) {
		auto filename = entry.path().filename().string();
		auto key = get_key(filename);

		dict[key].push_back(filename);
	}

	for (auto& [key, value] : dict) {
		sort_by_hour(value);
	}
}

std::pair<std::vector<std::string>, std::vector<std::string>>
BatchOrganizer::get_batch(const std::string& filename) const {
	auto key = get_key(filename);
	auto iter = dict.find(key);

	if (iter == dict.end()) return {};

	std::vector<std::string> ask {};
	std::vector<std::string> bid {};

	ask.reserve(iter->second.size() / 2 + 1);
	bid.reserve(iter->second.size() / 2 + 1);

	for (const auto& s : iter->second) {
		if (is_ask(s)) ask.push_back(s);
		if (is_bid(s)) bid.push_back(s);
	}

	return { std::move(ask), std::move(bid) };
}

void
BatchOrganizer::delete_key(const std::string& name) {
	const auto key = get_key(name);
	dict.erase(key);
}

bool
MultiFileReader::getline(std::string& out) {
	auto clear_stream = [&]() {
		curr.close();
		curr.clear();
	};

	while (true) {
		if (!curr.is_open()) {
			if (curr_idx >= files.size()) return false;

			clear_stream();
			auto path = dir / files[curr_idx];
			curr.open(path, std::ios::binary);

			if (!curr) throw std::runtime_error("Failed to open " + files[curr_idx]);
		}

		if (std::getline(curr, out)) return true;

		clear_stream();
		++curr_idx;
	}

	return false;
}