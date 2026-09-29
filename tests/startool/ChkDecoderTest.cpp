// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "formats/ChkDecoder.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    void appendLe16(
        std::vector<std::uint8_t> &data,
        std::uint16_t value
    )
    {
        data.push_back(static_cast<std::uint8_t>(value & 0xff));
        data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    }

    void appendLe32(
        std::vector<std::uint8_t> &data,
        std::uint32_t value
    )
    {
        data.push_back(static_cast<std::uint8_t>(value & 0xff));
        data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
        data.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
        data.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
    }

    std::vector<std::uint8_t> makeTerrainChk(
        std::uint16_t terrain
    )
    {
        std::vector<std::uint8_t> data{
            static_cast<std::uint8_t>('E'),
            static_cast<std::uint8_t>('R'),
            static_cast<std::uint8_t>('A'),
            static_cast<std::uint8_t>(' ')
        };

        // ERA
        appendLe32(data, 2);
        appendLe16(data, terrain);

        // DIM: minimal valid 1x1 map.
        data.push_back(static_cast<std::uint8_t>('D'));
        data.push_back(static_cast<std::uint8_t>('I'));
        data.push_back(static_cast<std::uint8_t>('M'));
        data.push_back(static_cast<std::uint8_t>(' '));

        appendLe32(data, 4);
        appendLe16(data, 1);
        appendLe16(data, 1);

        // MTXM: one tile for the 1x1 map.
        data.push_back(static_cast<std::uint8_t>('M'));
        data.push_back(static_cast<std::uint8_t>('T'));
        data.push_back(static_cast<std::uint8_t>('X'));
        data.push_back(static_cast<std::uint8_t>('M'));

        appendLe32(data, 2);
        appendLe16(data, 0);

        return data;
    }

    bool expectTerrain(
        std::uint16_t index,
        const std::string &expected
    )
    {
        ChkDecoder decoder;
        ChkMap map;
        std::string error;

        if (!decoder.decode(
            makeTerrainChk(index),
            map,
            error
        )) {
            std::cerr
                << "Could not decode terrain "
                << index
                << ": "
                << error
                << '\n';

            return false;
        }

        if (map.terrain != expected) {
            std::cerr
                << "Terrain "
                << index
                << " decoded as '"
                << map.terrain
                << "' instead of '"
                << expected
                << "'\n";

            return false;
        }

        return true;
    }
}

int main()
{
    const std::array<const char *, 8> terrainNames = {
        "badlands",
        "platform",
        "install",
        "ashworld",
        "jungle",
        "desert",
        "arctic",
        "twilight"
    };

    for (
        std::size_t index = 0;
        index < terrainNames.size();
        ++index
    ) {
        if (!expectTerrain(
            static_cast<std::uint16_t>(index),
            terrainNames[index]
        )) {
            return 1;
        }
    }

    ChkDecoder decoder;
    ChkMap map;
    std::string error;

    if (decoder.decode(
        makeTerrainChk(8),
        map,
        error
    )) {
        std::cerr
            << "Unsupported terrain index 8 was accepted\n";

        return 1;
    }

    if (
        error.find("unsupported terrain index") ==
        std::string::npos
    ) {
        std::cerr
            << "Unexpected error for unsupported terrain: "
            << error
            << '\n';

        return 1;
    }

    return 0;
}
