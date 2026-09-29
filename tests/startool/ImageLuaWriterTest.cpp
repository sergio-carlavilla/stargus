// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "converters/ImageLuaWriter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

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
}

int main()
{
    std::error_code filesystemError;

    const std::filesystem::path root =
        std::filesystem::temp_directory_path(
            filesystemError
        ) /
        "startool-image-lua-writer-test";

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

    const std::filesystem::path output =
        root / "image.lua";

    ImageLuaWriter writer;
    std::string error;

    if (!writer.write(
        output,
        "image_42_terran_marine",
        "graphics/unit/terran/marine.png",
        64,
        32,
        17,
        error
    )) {
        std::cerr
            << "ImageLuaWriter failed: "
            << error
            << '\n';
        return 1;
    }

    const std::string expected =
        "image_42_terran_marine_file = "
        "\"graphics/unit/terran/marine.png\"\n"
        "image_42_terran_marine_size = {64, 32}\n"
        "image_42_terran_marine_NumDirections = 17\n"
        "image_42_terran_marine = "
        "{\"file\", image_42_terran_marine_file, "
        "\"size\", image_42_terran_marine_size}\n"
        "image_42_terran_marine_var = "
        "{File = image_42_terran_marine_file, "
        "Size = image_42_terran_marine_size}\n";

    const std::string actual =
        readFile(output);

    if (actual != expected) {
        std::cerr
            << "Generated image Lua differs from expected output\n"
            << "--- expected ---\n"
            << expected
            << "--- actual ---\n"
            << actual;

        return 1;
    }

    error.clear();

    const std::filesystem::path missingParent =
        root /
        "missing" /
        "image.lua";

    if (writer.write(
        missingParent,
        "image_invalid",
        "graphics/invalid.png",
        1,
        1,
        1,
        error
    )) {
        std::cerr
            << "ImageLuaWriter unexpectedly created output "
            << "inside a missing directory\n";
        return 1;
    }

    std::filesystem::remove_all(root, filesystemError);

    return 0;
}
