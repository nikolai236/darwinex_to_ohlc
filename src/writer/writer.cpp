#include <vector>
#include <filesystem>

#include "duckdb.hpp"

#include "./writer.h"

namespace {

void
create_candles_table(const std::string& name, duckdb::Connection& connection) {
	const auto query =
		"CREATE TABLE IF NOT EXISTS " + name + " ("
		"	time   BIGINT PRIMARY KEY,"
		"	open   DOUBLE,"
		"	high   DOUBLE,"
		"	low    DOUBLE,"
		"	close  DOUBLE,"
		"	volume BIGINT"
		")";

	auto res = connection.Query(query);
	if (res->HasError()) {
		throw std::runtime_error(
			"Create query error: " + name + " " + res->GetError()
		);
	}
}

}

void
write_candles_to_db(const std::vector<Candle>& candles,
					const fs::path& db_path,
					const std::string& symbol)
{
	duckdb::DuckDB db(db_path);
	duckdb::Connection connection(db);

	const auto table_name   = "candles_" + symbol;
	const auto staging_name = table_name + "_staging";

	create_candles_table(table_name, connection);

	auto drop_res = connection.Query("DROP TABLE IF EXISTS " + staging_name);
	if (drop_res->HasError()) {
		auto msg = "Failed to drop staging table "
			+ staging_name 
			+ ": "
			+ drop_res->GetError();
		throw std::runtime_error(msg);
	}

	create_candles_table(staging_name, connection);

	{
		duckdb::Appender bulk_data(connection, staging_name);
		for (const auto& candle : candles) {
			bulk_data.BeginRow();
			bulk_data.Append(candle.time);
			bulk_data.Append(candle.open);
			bulk_data.Append(candle.high);
			bulk_data.Append(candle.low);
			bulk_data.Append(candle.close);
			bulk_data.Append(candle.tick_count);
			bulk_data.EndRow();
		}
		bulk_data.Close();
	}

	const auto merge_query =
		"INSERT INTO " + table_name +
		"	(time, open, high, low, close, volume) "
		"SELECT time, open, high, low, close, volume "
		"FROM " + staging_name + " "
		"ON CONFLICT (time) DO UPDATE SET "
		"  open   = EXCLUDED.open,"
		"  high   = EXCLUDED.high,"
		"  low    = EXCLUDED.low,"
		"  close  = EXCLUDED.close,"
		"  volume = EXCLUDED.volume";

	auto merge_res = connection.Query(merge_query);
	if (merge_res->HasError()) {
		auto msg = "Failed to merge staging into "
			+ table_name
			+ ": "
			+ merge_res->GetError();
		throw std::runtime_error(msg);
	}
}