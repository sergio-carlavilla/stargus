// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SourceTypes.h"

std::string_view toString(GameEdition edition)
{
    switch (edition) {
        case GameEdition::Classic:
            return "Classic";

        case GameEdition::BroodWar:
            return "Brood War";

        case GameEdition::Remastered:
            return "Remastered";

        case GameEdition::Unknown:
        default:
            return "Unknown";
    }
}

std::string_view toString(SourceFormat format)
{
    switch (format) {
        case SourceFormat::Mpq:
            return "MPQ";

        case SourceFormat::Casc:
            return "CASC";

        case SourceFormat::Directory:
            return "Directory";

        case SourceFormat::Unknown:
        default:
            return "Unknown";
    }
}
