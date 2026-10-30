// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "MpqVerifier.h"

#include "LogicalSource.h"
#include "MpqArchive.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    std::string lowercaseFilename(const std::filesystem::path &path)
    {
        std::string name = path.filename().string();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char value) {
            return static_cast<char>(std::tolower(value));
        });
        return name;
    }

    std::optional<std::filesystem::path> findCompanionInstaller(
        const std::filesystem::path &root,
        std::string_view gameMember
    )
    {
        const std::array<std::string_view, 4> names = {
            "install.exe", "starcraft.mpq", "installer tome.mpq", "starcraft archive"
        };
        std::vector<std::filesystem::path> files;
        std::error_code error;
        std::filesystem::directory_iterator iterator(root, error);
        if (error) {
            return std::nullopt;
        }
        const std::filesystem::directory_iterator end;
        while (iterator != end) {
            if (iterator->is_regular_file(error) && !error) {
                files.push_back(iterator->path());
            }
            error.clear();
            iterator.increment(error);
            if (error) {
                return std::nullopt;
            }
        }
        for (const std::string_view name : names) {
            for (const auto &path : files) {
                if (lowercaseFilename(path) != name) {
                    continue;
                }
                MpqArchive candidate(path);
                if (candidate.isOpen() && candidate.contains(gameMember)) {
                    return path;
                }
            }
        }
        return std::nullopt;
    }
}

bool MpqVerifier::verify(GameSource &source) const
{
    source.sources.clear();
    if (source.format != SourceFormat::Mpq) {
        return false;
    }

    MpqArchive archive(source.storage);
    if (!archive.isOpen()) {
        return false;
    }

    const bool hasBrooDat = archive.contains("files\\broodat.mpq");
    const bool hasStarDat = archive.contains("files\\stardat.mpq");
    if (hasBrooDat || hasStarDat) {
        source.edition = hasBrooDat ? GameEdition::BroodWar : GameEdition::Classic;
        source.sources.push_back(LogicalSource{
            "installer", SourceFormat::Mpq, SourceLocation::Filesystem, source.storage, {}
        });
        source.sources.push_back(LogicalSource{
            "game", SourceFormat::Mpq, SourceLocation::ArchiveMember, source.storage,
            hasBrooDat ? "files\\broodat.mpq" : "files\\stardat.mpq"
        });
        return true;
    }

    // Installed game archives are the game source itself, not CD containers.
    // A name alone is insufficient: reject unrelated MPQs renamed StarDat.
    const std::string name = lowercaseFilename(source.storage);
    if ((name != "stardat.mpq" && name != "broodat.mpq") ||
        !archive.contains("arr\\units.dat")) {
        return false;
    }
    source.edition = name == "broodat.mpq" ? GameEdition::BroodWar : GameEdition::Classic;
    source.sources.push_back(LogicalSource{
        "game", SourceFormat::Mpq, SourceLocation::Filesystem, source.storage, {}
    });
    const std::string_view member = source.edition == GameEdition::BroodWar
        ? "files\\broodat.mpq" : "files\\stardat.mpq";
    if (const auto installer = findCompanionInstaller(source.root, member)) {
        source.sources.push_back(LogicalSource{
            "installer", SourceFormat::Mpq, SourceLocation::Filesystem, *installer, {}
        });
    }
    return true;
}
