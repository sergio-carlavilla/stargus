// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef STARGUS_TOOLS_STARTOOL_CONVERTERS_UICONSOLECONVERTER_H
#define STARGUS_TOOLS_STARTOOL_CONVERTERS_UICONSOLECONVERTER_H

#include <filesystem>
#include <string>

class UiConsoleConverter
{
public:
    bool convert(
        const std::filesystem::path &inputPng,
        const std::filesystem::path &outputBase,
        int left,
        int right,
        std::string &error
    ) const;

private:
    bool callConvert(const std::string &command) const;
};

#endif // STARGUS_TOOLS_STARTOOL_CONVERTERS_UICONSOLECONVERTER_H
