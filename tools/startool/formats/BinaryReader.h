// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef BINARY_READER_H
#define BINARY_READER_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace BinaryReader
{

    inline std::uint16_t readLe16(
        const std::vector<unsigned char> &data,
        std::size_t offset
    )
    {
        return static_cast<std::uint16_t>(data[offset]) | static_cast<std::uint16_t>(data[offset + 1]) << 8;
    }

    inline std::uint32_t readLe32(
        const std::vector<unsigned char> &data,
        std::size_t offset
    )
    {
        return
            static_cast<std::uint32_t>(data[offset]) |
            static_cast<std::uint32_t>(data[offset + 1]) << 8 |
            static_cast<std::uint32_t>(data[offset + 2]) << 16 |
            static_cast<std::uint32_t>(data[offset + 3]) << 24;
    }

    inline std::uint16_t readBe16(
        const std::vector<unsigned char> &data,
        std::size_t offset
    )
    {
        return static_cast<std::uint16_t>(data[offset]) << 8 | static_cast<std::uint16_t>(data[offset + 1]);
    }

}

#endif // BINARY_READER_H
