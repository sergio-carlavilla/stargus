// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef TILESET_TO_PNG_CONVERTER_H
#define TILESET_TO_PNG_CONVERTER_H

#include "palette/Palette.h"

#include <filesystem>
#include <string>

class TilesetToPngConverter
{

    public:
        bool convert(
            const std::filesystem::path &vx4Input,
            const std::filesystem::path &vr4Input,
            const std::filesystem::path &output,
            const Palette &palette,
            std::string &error
        ) const;

};

#endif // TILESET_TO_PNG_CONVERTER_H
