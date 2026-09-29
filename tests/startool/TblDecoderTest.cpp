// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "formats/TblDecoder.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    void appendLe16(
        std::vector<unsigned char> &data,
        std::uint16_t value
    )
    {
        data.push_back(
            static_cast<unsigned char>(value & 0xff)
        );
        data.push_back(
            static_cast<unsigned char>((value >> 8) & 0xff)
        );
    }

    bool writeBytes(
        const std::filesystem::path &path,
        const std::vector<unsigned char> &data
    )
    {
        std::ofstream output(path, std::ios::binary);

        if (!output) {
            return false;
        }

        output.write(
            reinterpret_cast<const char *>(data.data()),
            static_cast<std::streamsize>(data.size())
        );

        return static_cast<bool>(output);
    }

    std::vector<unsigned char> makeTbl(
        const std::vector<std::string> &entries
    )
    {
        std::vector<unsigned char> data;

        appendLe16(
            data,
            static_cast<std::uint16_t>(entries.size())
        );

        std::uint16_t offset =
            static_cast<std::uint16_t>(
                2 + entries.size() * 2
            );

        for (const std::string &entry : entries) {
            appendLe16(data, offset);

            offset = static_cast<std::uint16_t>(
                offset + entry.size() + 1
            );
        }

        for (const std::string &entry : entries) {
            data.insert(
                data.end(),
                entry.begin(),
                entry.end()
            );
            data.push_back(0);
        }

        return data;
    }
}

int main()
{
    std::error_code filesystemError;

    const std::filesystem::path root =
        std::filesystem::temp_directory_path(
            filesystemError
        ) /
        "startool-tbl-decoder-test";

    if (filesystemError) {
        std::cerr
            << "Could not resolve temporary directory: "
            << filesystemError.message()
            << '\n';
        return 1;
    }

    std::filesystem::remove_all(root, filesystemError);
    filesystemError.clear();
    std::filesystem::create_directories(root, filesystemError);

    if (filesystemError) {
        std::cerr
            << "Could not create temporary directory: "
            << filesystemError.message()
            << '\n';
        return 1;
    }

    const std::filesystem::path validPath =
        root / "valid.tbl";

    const std::vector<std::string> sourceEntries{
        "Unit\\Marine.GRP",
        "thingy\\tileset\\AshWorld\\lava.grp",
        "Line\nBreak.grp"
    };

    if (!writeBytes(
        validPath,
        makeTbl(sourceEntries)
    )) {
        std::cerr << "Could not write TBL fixture\n";
        return 1;
    }

    TblDecoder decoder;
    std::vector<TblEntry> entries;
    std::string error;

    if (!decoder.decode(
        validPath,
        entries,
        error
    )) {
        std::cerr
            << "Valid TBL fixture failed: "
            << error
            << '\n';
        return 1;
    }

    const std::vector<std::string> expected{
        "Unit\\Marine.GRP",
        "thingy\\tileset\\AshWorld\\lava.grp",
        "Line Break.grp"
    };

    if (entries.size() != expected.size()) {
        std::cerr
            << "Expected "
            << expected.size()
            << " TBL entries, got "
            << entries.size()
            << '\n';
        return 1;
    }

    for (
        std::size_t index = 0;
        index < expected.size();
        ++index
    ) {
        if (entries[index].name != expected[index]) {
            std::cerr
                << "Unexpected TBL entry "
                << index
                << ": '"
                << entries[index].name
                << "'\n";
            return 1;
        }
    }

    const std::filesystem::path invalidPath =
        root / "invalid.tbl";

    std::vector<unsigned char> invalid{
        1, 0,
        0xff, 0x7f
    };

    if (!writeBytes(invalidPath, invalid)) {
        std::cerr << "Could not write invalid TBL fixture\n";
        return 1;
    }

    entries.clear();
    error.clear();

    if (decoder.decode(
        invalidPath,
        entries,
        error
    )) {
        std::cerr
            << "TBL with out-of-range offset was accepted\n";
        return 1;
    }

    std::filesystem::remove_all(root, filesystemError);

    return 0;
}
