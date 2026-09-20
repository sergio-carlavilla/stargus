// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "MpqVerifier.h"

#include "LogicalSource.h"
#include "MpqArchive.h"

bool MpqVerifier::verify(GameSource &source) const
{
    MpqArchive archive(source.storage);
    if (!archive.isOpen()) {
        return false;
    }

    source.sources.clear();
    source.sources.push_back(
        LogicalSource{
            "installer",
            SourceFormat::Mpq,
            SourceLocation::Filesystem,
            source.storage,
            {}
        }
    );

    // Were not using a local path of the machine, this is inspecting inside the mpq
    // so we need to use Microsoft-style paths
    const bool hasBrooDat = archive.contains("files\\broodat.mpq");
    const bool hasStarDat = archive.contains("files\\stardat.mpq");

    if (hasBrooDat) {
        source.edition = GameEdition::BroodWar;

        source.sources.push_back(
            LogicalSource{
                "game",
                SourceFormat::Mpq,
                SourceLocation::ArchiveMember,
                source.storage,
                "files\\broodat.mpq"
            }
        );
    } else if (hasStarDat) {
        source.edition = GameEdition::Classic;
        source.sources.push_back(
            LogicalSource{
                "game",
                SourceFormat::Mpq,
                SourceLocation::ArchiveMember,
                source.storage,
                "files\\stardat.mpq"
            }
        );
    } else {
        return false;
    }

    return true;
}
