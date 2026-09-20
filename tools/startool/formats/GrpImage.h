// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef GRP_IMAGE_H
#define GRP_IMAGE_H

#include <cstdint>
#include <vector>

struct GrpFrame
{
    std::uint8_t xOffset = 0;
    std::uint8_t yOffset = 0;
    std::uint8_t width = 0;
    std::uint8_t height = 0;

    std::uint32_t dataOffset = 0;

    std::vector<std::uint8_t> pixels;
};

struct GrpImage
{
    std::uint16_t maximumWidth = 0;
    std::uint16_t maximumHeight = 0;

    bool uncompressed = false;

    std::vector<GrpFrame> frames;
};

#endif // GRP_IMAGE_H
