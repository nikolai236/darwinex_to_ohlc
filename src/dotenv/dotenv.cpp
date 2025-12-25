#include <iostream>
#include <fstream>
#include <cstdlib>

#include "dotenv.h"

namespace {
    void trim_quotes(std::string& s) {
        if (s.size() < 2 || s.front() != '"' || s.back() != '"') return;
        s = s.substr(1, s.size() - 2);
    }
}

void
load_dotenv(const std::string& path) {
    std::ifstream file(path);
    if (!file) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        trim_quotes(val);

#ifdef _WIN32
        _putenv_s(key.c_str(), val.c_str());
#else
        setenv(key.c_str(), val.c_str(), 1);
#endif
    }
}