// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PCX_PALETTE_LOADER_H
#define PCX_PALETTE_LOADER_H

#include "Palette.h"
#include "manifest/PaletteDefinition.h"

#include <filesystem>
#include <string>

class PcxPaletteLoader
{

    public:
        bool load(
            const std::filesystem::path &input,
            const PaletteDefinition &definition,
            Palette &palette,
            std::string &error
        ) const;

};

#endif // PCX_PALETTE_LOADER_H
