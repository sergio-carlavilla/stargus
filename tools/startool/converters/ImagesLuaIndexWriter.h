// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef STARGUS_TOOLS_STARTOOL_CONVERTERS_IMAGESLUAINDEXWRITER_H
#define STARGUS_TOOLS_STARTOOL_CONVERTERS_IMAGESLUAINDEXWRITER_H

#include <filesystem>
#include <string>

class ImagesLuaIndexWriter
{
    public:
        bool write(
            const std::filesystem::path &imagesDirectory,
            std::string &error
        ) const;
};

#endif
