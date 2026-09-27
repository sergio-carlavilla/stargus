// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef CHK_LUA_WRITER_H
#define CHK_LUA_WRITER_H

#include "formats/ChkMap.h"
#include "manifest/UnitDefinition.h"

#include <filesystem>
#include <string>
#include <vector>

class ChkLuaWriter
{

    public:
        bool write(
            const ChkMap &map,
            const std::vector<UnitDefinition> &units,
            const std::filesystem::path &outputBase,
            std::string &error
        ) const;

};

#endif // CHK_LUA_WRITER_H
