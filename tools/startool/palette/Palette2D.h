// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PALETTE_2D_H
#define PALETTE_2D_H

#include "Palette.h"

#include <cstddef>
#include <vector>

class Palette2D
{

    public:
        static constexpr std::size_t Width = 256;

        Palette2D() = default;

        explicit Palette2D(std::size_t height) : colors(height * Width) { }

        std::size_t height() const
        {
            return colors.size() / Width;
        }

        PaletteColor &at(std::size_t column, std::size_t row)
        {
            return colors.at(row * Width + column);
        }

        const PaletteColor &at(std::size_t column, std::size_t row) const
        {
            return colors.at(row * Width + column);
        }

    private:
        std::vector<PaletteColor> colors;

};

#endif // PALETTE_2D_H
