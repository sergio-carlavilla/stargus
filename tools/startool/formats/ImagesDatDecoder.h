// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef IMAGES_DAT_DECODER_H
#define IMAGES_DAT_DECODER_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct ImagesDatRecord
{
    std::uint32_t grp = 0;
    bool gfxTurns = false;
    bool clickable = false;
    bool useFullIscript = false;
    bool drawIfCloaked = false;
    std::uint8_t drawFunction = 0;
    std::uint8_t remapping = 0;
    std::uint32_t iscript = 0;
    std::uint32_t shieldOverlay = 0;
    std::uint32_t attackOverlay = 0;
    std::uint32_t damageOverlay = 0;
    std::uint32_t specialOverlay = 0;
    std::uint32_t landingDustOverlay = 0;
    std::uint32_t liftOffDustOverlay = 0;
};

class ImagesDatDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            std::vector<ImagesDatRecord> &records,
            std::string &error
        ) const;

};

#endif // IMAGES_DAT_DECODER_H
