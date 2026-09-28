// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef STARGUS_TOOLS_STARTOOL_CONVERTERS_PANELTOPNGCONVERTER_H
#define STARGUS_TOOLS_STARTOOL_CONVERTERS_PANELTOPNGCONVERTER_H

#include <filesystem>
#include <string>

class PanelToPngConverter
{
public:
    bool convert(
        const std::filesystem::path &output,
        int width,
        int height,
        std::string &error
    ) const;
};

#endif // STARGUS_TOOLS_STARTOOL_CONVERTERS_PANELTOPNGCONVERTER_H
