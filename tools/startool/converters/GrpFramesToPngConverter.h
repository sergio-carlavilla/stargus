// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef GRP_FRAMES_TO_PNG_CONVERTER_H
#define GRP_FRAMES_TO_PNG_CONVERTER_H

#include "palette/Palette.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

class GrpFramesToPngConverter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            const Palette &palette,
            const std::vector<std::size_t> &frameIndices,
            bool rgba,
            std::string &error
        ) const;

};

#endif // GRP_FRAMES_TO_PNG_CONVERTER_H
