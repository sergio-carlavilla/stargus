// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "TblDecoder.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace
{

    std::uint16_t readLittleEndian16(
        const std::vector<unsigned char> &data,
        std::size_t offset
    )
    {
        return static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(data[offset]) |
            static_cast<std::uint16_t>(data[offset + 1]) << 8
        );
    }

}

bool TblDecoder::decode(
    const std::filesystem::path &input,
    std::vector<TblEntry> &entries,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open TBL file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    if (data.size() < 2) {
        error = "TBL file is too small: " + input.string();
        return false;
    }

    const std::size_t entryCount = readLittleEndian16(data, 0);
    const std::size_t headerSize = 2 + entryCount * 2;

    if (headerSize > data.size()) {
        error = "TBL offset table exceeds file size: " + input.string();
        return false;
    }

    std::vector<std::uint16_t> offsets;
    offsets.reserve(entryCount);

    for (std::size_t index = 0; index < entryCount; ++index) {
        const std::uint16_t offset = readLittleEndian16(data, 2 + index * 2);

        if (offset >= data.size()) {
            error = "TBL entry offset exceeds file size: " + std::to_string(index);
            return false;
        }

        offsets.push_back(offset);
    }

    entries.clear();
    entries.reserve(entryCount);

    for (std::size_t index = 0; index < entryCount; ++index) {
        const std::size_t start = offsets[index];
        const std::size_t end = index + 1 < entryCount ? static_cast<std::size_t>(offsets[index + 1]) : data.size();

        if (end < start || end > data.size()) {
            error = "TBL entry range is invalid: " + std::to_string(index);
            return false;
        }

        TblEntry entry;

        for (std::size_t position = start; position < end; ++position) {
            const unsigned char value = data[position];

            if (value == 0) {
                break;
            }

            if (value == '\n') {
                entry.name += ' ';
            } else if (value >= 32 || value == '*') {
                entry.name += static_cast<char>(value);
            }
        }

        entries.push_back(std::move(entry));
    }

    return true;
}
