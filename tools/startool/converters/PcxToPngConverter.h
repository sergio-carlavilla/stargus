// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PCX_TO_PNG_CONVERTER_H
#define PCX_TO_PNG_CONVERTER_H

#include <filesystem>
#include <string>

class PcxToPngConverter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            std::string &error
        ) const;

};

#endif // PCX_TO_PNG_CONVERTER_H
