// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "CascVerifier.h"

#include "CascSourceReader.h"
#include "LogicalSource.h"

bool CascVerifier::verify(GameSource &source) const
{
    if (source.format != SourceFormat::Casc) {
        return false;
    }

    LogicalSource installerSource{
        "installer",
        SourceFormat::Casc,
        SourceLocation::Filesystem,
        source.storage,
        {}
    };

    LogicalSource gameSource{
        "game",
        SourceFormat::Casc,
        SourceLocation::Filesystem,
        source.storage,
        {}
    };

    CascSourceReader reader(gameSource);

    if (!reader.isOpen()) {
        return false;
    }

    /*
     * Verify that this is a usable StarCraft CASC storage rather than merely
     * a directory containing .build.info
     */
    if (!reader.contains("arr/units.dat") || !reader.contains("rez/stat_txt.tbl")) {
        return false;
    }

    source.sources.clear();

    /*
     * Modern Battle.net installations use a unified CASC storage. The
     * logical "installer" and "game" sources therefore point to the same
     * physical storage. CascSourceReader is responsible for resolving
     * legacy manifest paths to their modern CASC counterparts
     */
    source.sources.push_back(installerSource);
    source.sources.push_back(gameSource);

    // Battle.net keeps legacy .scm/.scx maps as loose files under Maps/
    // Expose the installation directory independently from CASC archives
    source.sources.push_back(LogicalSource{
        "files",
        SourceFormat::Directory,
        SourceLocation::Filesystem,
        source.root,
        {}
    });

    return true;
}
