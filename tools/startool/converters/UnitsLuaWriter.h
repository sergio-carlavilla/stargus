// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef UNITS_LUA_WRITER_H
#define UNITS_LUA_WRITER_H

#include "UnitDataResolver.h"
#include "manifest/UnitDefinition.h"

#include <filesystem>
#include <string>
#include <vector>

class UnitsLuaWriter
{

    public:
        bool write(
            const std::vector<UnitDefinition> &units,
            UnitDataResolver &resolver,
            const std::filesystem::path &output,
            std::string &error
        ) const;

};

#endif // UNITS_LUA_WRITER_H
