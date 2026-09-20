// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef WAV_TO_OGG_CONVERTER_H
#define WAV_TO_OGG_CONVERTER_H

#include <filesystem>
#include <string>

class WavToOggConverter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            std::string &error
        ) const;

};

#endif // WAV_TO_OGG_CONVERTER_H
