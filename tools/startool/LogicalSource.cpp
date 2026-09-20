// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "LogicalSource.h"

std::string_view toString(SourceLocation location)
{
    switch (location) {
        case SourceLocation::Filesystem:
            return "filesystem";

        case SourceLocation::ArchiveMember:
            return "archive member";

        default:
            return "unknown";
    }
}
