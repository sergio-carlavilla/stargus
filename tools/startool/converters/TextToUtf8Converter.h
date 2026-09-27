// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef TEXT_TO_UTF8_CONVERTER_H
#define TEXT_TO_UTF8_CONVERTER_H

#include <filesystem>
#include <string>

class TextToUtf8Converter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            std::string &error
        ) const;

};

#endif // TEXT_TO_UTF8_CONVERTER_H
