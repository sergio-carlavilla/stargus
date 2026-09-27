// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef UNIT_LUA_WRITER_H
#define UNIT_LUA_WRITER_H

#include "UnitMetadata.h"

#include <filesystem>
#include <string>

class UnitLuaWriter
{

    public:
        std::string render(const UnitMetadata &metadata) const;

        bool write(
            const UnitMetadata &metadata,
            const std::filesystem::path &output,
            std::string &error
        ) const;

};

#endif // UNIT_LUA_WRITER_H
