// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "converters/PortraitLuaWriter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    std::string readFile(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary);

        return std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );
    }

    bool expectPortrait(
        const std::filesystem::path &root,
        const std::string &id,
        const std::vector<std::string> &assets,
        const std::string &expected
    )
    {
        const std::filesystem::path output = root / ("portrait-" + id + ".lua");

        PortraitLuaWriter writer;
        std::string error;

        if (!writer.writePortrait(id, assets, output, error)) {
            std::cerr << "PortraitLuaWriter failed: " << error << '\n';
            return false;
        }

        const std::string actual = readFile(output);

        if (actual != expected) {
            std::cerr << "Unexpected portrait Lua:\n" << actual << "\nExpected:\n" << expected << '\n';
            return false;
        }

        return true;
    }
}

int main()
{
    std::error_code filesystemError;

    const std::filesystem::path root =
        std::filesystem::temp_directory_path(filesystemError) /
        "startool-portrait-lua-writer-test";

    if (filesystemError) {
        std::cerr << filesystemError.message() << '\n';
        return 1;
    }

    std::filesystem::remove_all(root, filesystemError);
    filesystemError.clear();
    std::filesystem::create_directories(root, filesystemError);

    if (filesystemError) {
        std::cerr << filesystemError.message() << '\n';
        return 1;
    }

    // One idle video: written with its real name, once
    if (!expectPortrait(
        root,
        "zlarva",
        { "videos/portrait/zlarva/zlafid00.mng", "talking", "videos/portrait/zlarva/zlatlk00.mng" },
        "portrait_zlarva = {"
        "\"videos/portrait/zlarva/zlafid00.mng\", "
        "\"talking\", "
        "\"videos/portrait/zlarva/zlatlk00.mng\"}"
    )) {
        return 1;
    }

    // Several idle and talking videos keep their order
    if (!expectPortrait(
        root,
        "tmarine",
        {
            "videos/portrait/tmarine/tmafid00.mng",
            "videos/portrait/tmarine/tmafid01.mng",
            "talking",
            "videos/portrait/tmarine/tmatlk00.mng",
            "videos/portrait/tmarine/tmatlk01.mng"
        },
        "portrait_tmarine = {"
        "\"videos/portrait/tmarine/tmafid00.mng\", "
        "\"videos/portrait/tmarine/tmafid01.mng\", "
        "\"talking\", "
        "\"videos/portrait/tmarine/tmatlk00.mng\", "
        "\"videos/portrait/tmarine/tmatlk01.mng\"}"
    )) {
        return 1;
    }

    // An existing portrait Lua is never overwritten
    {
        PortraitLuaWriter writer;
        std::string error;

        if (writer.writePortrait("zlarva", { "other.mng", "talking", "other.mng" }, root / "portrait-zlarva.lua", error)) {
            std::cerr << "PortraitLuaWriter overwrote an existing portrait Lua\n";
            return 1;
        }

        if (readFile(root / "portrait-zlarva.lua").find("zlafid00.mng") == std::string::npos) {
            std::cerr << "Existing portrait Lua was modified\n";
            return 1;
        }
    }

    // The loader keeps one Load per portdata.dat reference, repeats included
    {
        PortraitLuaWriter writer;
        std::string error;
        const std::filesystem::path loader = root / "luagen-portrait.lua";

        if (!writer.writeLoader(
            {
                "luagen/portrait/portrait-tmarine.lua",
                "luagen/portrait/portrait-zlarva.lua",
                "luagen/portrait/portrait-tmarine.lua"
            },
            loader,
            error
        )) {
            std::cerr << "PortraitLuaWriter loader failed: " << error << '\n';
            return 1;
        }

        const std::string expected =
            "Load(\"luagen/portrait/portrait-tmarine.lua\")\n"
            "Load(\"luagen/portrait/portrait-zlarva.lua\")\n"
            "Load(\"luagen/portrait/portrait-tmarine.lua\")\n";

        if (readFile(loader) != expected) {
            std::cerr << "Unexpected portrait loader:\n" << readFile(loader) << '\n';
            return 1;
        }
    }

    std::filesystem::remove_all(root, filesystemError);

    std::cout << "PortraitLuaWriterTest passed\n";
    return 0;
}
