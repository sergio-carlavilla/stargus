// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef FONT_TO_PNG_CONVERTER_H
#define FONT_TO_PNG_CONVERTER_H

#include "palette/Palette.h"

#include <filesystem>
#include <string>

class FontToPngConverter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            const Palette &palette,
            std::string &error
        ) const;

};

#endif // FONT_TO_PNG_CONVERTER_H
