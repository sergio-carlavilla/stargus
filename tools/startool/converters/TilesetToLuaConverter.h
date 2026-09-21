// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef TILESET_TO_LUA_CONVERTER_H
#define TILESET_TO_LUA_CONVERTER_H

#include <filesystem>
#include <string>

class TilesetToLuaConverter
{

    public:
        bool convert(
            const std::filesystem::path &cv5Input,
            const std::filesystem::path &vf4Input,
            const std::filesystem::path &output,
            const std::string &tilesetName,
            const std::string &image,
            std::string &error
        ) const;

};

#endif // TILESET_TO_LUA_CONVERTER_H
