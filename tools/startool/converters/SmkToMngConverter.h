// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SMK_TO_MNG_CONVERTER_H
#define SMK_TO_MNG_CONVERTER_H

#include <filesystem>
#include <string>

class SmkToMngConverter
{

    public:
        bool convert(
            const std::filesystem::path &input,
            const std::filesystem::path &output,
            std::string &error
        ) const;

    private:
        bool convertFramesToMng(
            const std::filesystem::path &framesDirectory,
            const std::filesystem::path &output
        ) const;

};

#endif // SMK_TO_MNG_CONVERTER_H
