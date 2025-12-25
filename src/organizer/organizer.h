#pragma once

#include <filesystem>
#include <vector>
#include <fstream>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

class BatchOrganizer {
public:
	explicit
	BatchOrganizer(const fs::path& directory) {
		populate_dict(directory);
	}

	std::pair<std::vector<std::string>, std::vector<std::string>>
	get_batch(const std::string& filename) const;

	void
	delete_key(const std::string& key);

private:
	std::unordered_map<std::string, std::vector<std::string>> dict;

	bool
	is_ask(const std::string& filename) const;

	bool
	is_bid(const std::string& filename) const;

	std::string
	get_key(const std::string& filename) const;

	int
	extract_hour(const std::string& filename) const;

	void
	sort_by_hour(std::vector<std::string>& filenames) const;

	void
	populate_dict(const fs::path& directory);
};

class MultiFileReader {
private:
	const std::vector<std::string> files;
	std::ifstream curr;
	size_t curr_idx;
	fs::path dir;

public:
	MultiFileReader(const std::vector<std::string>& files, const fs::path& dir):
		files(files), curr_idx(0), dir{dir} {}

	MultiFileReader(MultiFileReader&&) = default;

	bool
	getline(std::string& out);
};