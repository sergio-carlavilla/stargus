// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef SOURCETYPES_H
#define SOURCETYPES_H

#include <string_view>

enum class GameEdition
{
    Unknown,
    Classic,
    BroodWar,
    Remastered
};

enum class SourceFormat
{
    Unknown,
    Mpq, // Used in the Classic and BroodWar installations
    Casc, // Used in the new Remastered edition
    Directory
};

std::string_view toString(GameEdition edition);
std::string_view toString(SourceFormat format);

#endif // SOURCETYPES_H
