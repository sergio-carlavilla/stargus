// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PCX_2D_PALETTE_LOADER_H
#define PCX_2D_PALETTE_LOADER_H

#include "Palette2D.h"
#include "manifest/PaletteDefinition.h"

#include <filesystem>
#include <string>

class Pcx2DPaletteLoader
{

    public:
        bool load(
            const std::filesystem::path &input,
            const PaletteDefinition &definition,
            Palette2D &palette,
            std::string &error
        ) const;

};

#endif // PCX_2D_PALETTE_LOADER_H
