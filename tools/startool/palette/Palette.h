// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PALETTE_H
#define PALETTE_H

#include <array>
#include <cstdint>

struct PaletteColor
{
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
};

using Palette = std::array<PaletteColor, 256>;

#endif // PALETTE_H
