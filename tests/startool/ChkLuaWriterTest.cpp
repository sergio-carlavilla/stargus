// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "converters/ChkLuaWriter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    std::string readFile(
        const std::filesystem::path &path
    )
    {
        std::ifstream input(
            path,
            std::ios::binary
        );

        return std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );
    }

    bool testTerrainOutput(
        const std::filesystem::path &root,
        const std::string &terrain,
        const std::string &expectedLuaName
    )
    {
        const std::filesystem::path directory =
            root / terrain;

        std::error_code filesystemError;

        std::filesystem::create_directories(
            directory,
            filesystemError
        );

        if (filesystemError) {
            std::cerr
                << "Could not create test directory: "
                << filesystemError.message()
                << '\n';

            return false;
        }

        ChkMap map;
        map.width = 1;
        map.height = 1;
        map.terrain = terrain;
        map.description = "Startool CHK regression fixture";
        map.tiles = {123};

        const std::vector<UnitDefinition> units{
            UnitDefinition{
                0,
                "unit-startool-test",
                true
            }
        };

        const std::filesystem::path outputBase =
            directory / "scenario.";

        ChkLuaWriter writer;
        std::string error;

        if (!writer.write(
            map,
            units,
            outputBase,
            error
        )) {
            std::cerr
                << "CHK Lua writer failed for terrain '"
                << terrain
                << "': "
                << error
                << '\n';

            return false;
        }

        const std::filesystem::path sms =
            directory / "scenario.sms";

        const std::string content =
            readFile(sms);

        const std::string expected =
            "LoadTileModels(\"luagen/tilesets/" +
            expectedLuaName +
            ".lua\")";

        if (content.find(expected) == std::string::npos) {
            std::cerr
                << "Generated SMS for terrain '"
                << terrain
                << "' does not contain expected reference: "
                << expected
                << '\n';

            return false;
        }

        if (
            terrain == "arctic" &&
            content.find(
                "luagen/tilesets/arctic.lua"
            ) != std::string::npos
        ) {
            std::cerr
                << "Arctic terrain still references "
                << "luagen/tilesets/arctic.lua\n";

            return false;
        }

        return true;
    }
}

int main()
{
    std::error_code filesystemError;

    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path(
            filesystemError
        ) /
        "startool-chk-lua-writer-test";

    if (filesystemError) {
        std::cerr
            << "Could not resolve temporary directory: "
            << filesystemError.message()
            << '\n';

        return 1;
    }

    std::filesystem::remove_all(
        tempRoot,
        filesystemError
    );

    filesystemError.clear();

    const bool arcticOk =
        testTerrainOutput(
            tempRoot,
            "arctic",
            "ice"
        );

    const bool badlandsOk =
        testTerrainOutput(
            tempRoot,
            "badlands",
            "badlands"
        );

    std::filesystem::remove_all(
        tempRoot,
        filesystemError
    );

    if (!arcticOk || !badlandsOk) {
        return 1;
    }

    return 0;
}
