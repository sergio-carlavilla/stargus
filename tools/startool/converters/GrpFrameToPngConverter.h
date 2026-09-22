// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef GRP_FRAME_TO_PNG_CONVERTER_H
#define GRP_FRAME_TO_PNG_CONVERTER_H

#include "palette/Palette.h"

#include <cstddef>
#include <filesystem>
#include <string>

class GrpFrameToPngConverter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            const Palette &palette,
            std::size_t frameIndex,
            bool rgba,
            std::string &error
        ) const;

};

#endif // GRP_FRAME_TO_PNG_CONVERTER_H
