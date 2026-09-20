// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef GRP_TO_PNG_CONVERTER_H
#define GRP_TO_PNG_CONVERTER_H

#include "palette/Palette.h"

#include <filesystem>
#include <string>

class GrpToPngConverter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            const Palette &palette,
            bool rgba,
            std::string &error
        ) const;

};

#endif // GRP_TO_PNG_CONVERTER_H
