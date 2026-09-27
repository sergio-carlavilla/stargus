// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef UNIT_DATA_RESOLVER_H
#define UNIT_DATA_RESOLVER_H

#include "UnitMetadata.h"

#include <cstddef>
#include <filesystem>
#include <string>

struct UnitDataFiles
{
    std::filesystem::path unitsDat;
    std::filesystem::path flingyDat;
    std::filesystem::path spritesDat;

    std::filesystem::path imagesDat;
    std::filesystem::path imagesTbl;

    std::filesystem::path portdataDat;
    std::filesystem::path portdataTbl;

    std::filesystem::path sfxdataDat;
    std::filesystem::path sfxdataTbl;

    std::filesystem::path statTxtTbl;
};

class UnitDataResolver
{

    public:
        bool resolve(
            const UnitDataFiles &files,
            std::size_t unitId,
            const std::string &ident,
            UnitMetadata &metadata,
            std::string &error
        ) const;

};

#endif // UNIT_DATA_RESOLVER_H
