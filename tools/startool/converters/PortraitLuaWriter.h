// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PORTRAIT_LUA_WRITER_H
#define PORTRAIT_LUA_WRITER_H

#include <filesystem>
#include <string>
#include <vector>

class PortraitLuaWriter
{
    public:
        bool writePortrait(
            const std::string &id,
            const std::vector<std::string> &assets,
            const std::filesystem::path &output,
            std::string &error
        ) const;

        bool writeLoader(
            const std::vector<std::string> &portraitLuaFiles,
            const std::filesystem::path &output,
            std::string &error
        ) const;
};

#endif // PORTRAIT_LUA_WRITER_H
