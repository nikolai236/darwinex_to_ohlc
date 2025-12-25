cmake_minimum_required(VERSION 3.20)
project(candles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----- Options -----
option(USE_DUCKDB "Enable DuckDB (Parquet output + SQL querying)" ON)

# ----- Main executable -----
add_executable(candles
    src/main.cpp
    src/dotenv/dotenv.cpp
    src/ftp/ftp_client.cpp
    src/decompress/decompress.cpp
    src/transform/transform.cpp
    src/organizer/organizer.cpp
    src/writer/writer.cpp
)

# Warnings (nice defaults for g++)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(candles PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ----- zlib (for .gz) -----
find_package(ZLIB REQUIRED)
target_link_libraries(candles PRIVATE ZLIB::ZLIB)

# ----- libcurl (for FTP download) -----
find_package(CURL REQUIRED)
target_link_libraries(candles PRIVATE CURL::libcurl)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(candles PRIVATE spdlog::spdlog)

if (USE_DUCKDB)
    include(FetchContent)

    FetchContent_Declare(
        duckdb
        GIT_REPOSITORY https://github.com/duckdb/duckdb.git
        GIT_TAG v1.1.3
    )
    FetchContent_MakeAvailable(duckdb)

    target_link_libraries(candles PRIVATE duckdb)
    target_compile_definitions(candles PRIVATE USE_DUCKDB=1)
endif()