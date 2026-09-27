// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SPRITES_DAT_DECODER_H
#define SPRITES_DAT_DECODER_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct SpritesDatRecord
{
    std::uint16_t image = 0;
};

class SpritesDatDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            std::vector<SpritesDatRecord> &records,
            std::string &error
        ) const;

};

#endif // SPRITES_DAT_DECODER_H
