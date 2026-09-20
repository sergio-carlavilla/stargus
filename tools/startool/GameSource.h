// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef GAMESOURCE_H
#define GAMESOURCE_H

#include "LogicalSource.h"
#include "SourceTypes.h"

#include <filesystem>
#include <vector>

struct GameSource
{
    std::filesystem::path root;
    std::filesystem::path storage;

    GameEdition edition = GameEdition::Unknown;
    SourceFormat format = SourceFormat::Unknown;

    std::vector<LogicalSource> sources;
};

#endif // GAMESOURCE_H
