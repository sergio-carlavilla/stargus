// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef DDS_TO_PNG_CONVERTER_H
#define DDS_TO_PNG_CONVERTER_H

#include <filesystem>
#include <string>

class DdsToPngConverter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            std::string &error,
            int cropX = -1,
            int cropY = -1,
            int cropWidth = -1,
            int cropHeight = -1
        ) const;

};

#endif // DDS_TO_PNG_CONVERTER_H
