#include <vector>
#include <filesystem>

#include "../transform/transform.h"

void
write_candles_to_db(const std::vector<Candle>& candles,
					const fs::path& db_path,
					const std::string& symbol);