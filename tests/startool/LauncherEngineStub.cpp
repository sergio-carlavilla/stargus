// SPDX-License-Identifier: GPL-2.0-only
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

int main(int argc, char **argv)
{
    const char *log = std::getenv("STARGUS_TEST_ENGINE_LOG");
    if (!log) return 1;
    std::vector<std::string> arguments(argv, argv + argc);
    std::ofstream stream(log);
    stream << nlohmann::json(arguments).dump();
    return stream ? 0 : 1;
}
