// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SFX_DATA_DECODER_H
#define SFX_DATA_DECODER_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct SfxDataRecord
{
    std::uint32_t soundFile = 0;

    std::uint8_t unknown1 = 0;
    std::uint8_t unknown2 = 0;
    std::uint16_t unknown3 = 0;
    std::uint8_t unknown4 = 0;
};

class SfxDataDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            std::vector<SfxDataRecord> &records,
            std::string &error
        ) const;

};

#endif // SFX_DATA_DECODER_H
