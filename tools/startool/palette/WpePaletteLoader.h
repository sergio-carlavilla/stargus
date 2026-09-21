// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef WPE_PALETTE_LOADER_H
#define WPE_PALETTE_LOADER_H

#include "Palette.h"
#include "manifest/PaletteDefinition.h"

#include <filesystem>
#include <string>

class WpePaletteLoader
{

    public:
        bool load(
            const std::filesystem::path &input,
            const PaletteDefinition &definition,
            Palette &palette,
            std::string &error
        ) const;

};

#endif // WPE_PALETTE_LOADER_H
