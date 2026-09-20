// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef LOGICALSOURCE_H
#define LOGICALSOURCE_H

#include "SourceTypes.h"

#include <filesystem>
#include <string>
#include <string_view>

enum class SourceLocation
{
    Filesystem,
    ArchiveMember
};

struct LogicalSource
{
    std::string id;

    SourceFormat format = SourceFormat::Unknown;
    SourceLocation location = SourceLocation::Filesystem;

    std::filesystem::path storage;
    std::string member;
};

std::string_view toString(SourceLocation location);

#endif // LOGICALSOURCE_H
