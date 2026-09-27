// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef IMAGE_LUA_WRITER_H
#define IMAGE_LUA_WRITER_H

#include <cstddef>
#include <filesystem>
#include <string>

class ImageLuaWriter
{

    public:
        bool write(
            const std::filesystem::path &output,
            const std::string &imageId,
            const std::string &pngRelativePath,
            std::size_t width,
            std::size_t height,
            int numDirections,
            std::string &error
        ) const;

};

#endif // IMAGE_LUA_WRITER_H
