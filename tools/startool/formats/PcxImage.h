// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PCX_IMAGE_H
#define PCX_IMAGE_H

#include "palette/Palette.h"

#include <cstdint>
#include <vector>

struct PcxImage
{
    std::uint16_t width = 0;
    std::uint16_t height = 0;

    std::vector<std::uint8_t> pixels;
    Palette palette;
};

#endif // PCX_IMAGE_H
