// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SourceDetector.h"

#include <algorithm>
#include <cctype>
#include <system_error>

std::optional<GameSource> SourceDetector::detect(const std::filesystem::path &path) const
{
    std::error_code error;

    const bool isDirectory = std::filesystem::is_directory(path, error);

    if (error) {
        return std::nullopt;
    }

    const bool isFile = std::filesystem::is_regular_file(path, error);
    if (error) {
        return std::nullopt;
    }

    /*
     * A file may itself be an MPQ archive
     */
    if (isFile) {
        const std::string filename = lowercase(path.filename().string());

        GameSource source;

        source.root = path.parent_path();
        source.storage = path;
        source.format = SourceFormat::Mpq;

        if (filename == "broodat.mpq") {
            source.edition = GameEdition::BroodWar;
        } else if (filename == "stardat.mpq") {
            source.edition = GameEdition::Classic;
        } else {
            source.edition = GameEdition::Unknown;
        }

        return source;
    }

    if (!isDirectory) {
        return std::nullopt;
    }

    const std::filesystem::path buildInfo = path / ".build.info";

    if (std::filesystem::is_regular_file(buildInfo, error)) {
        if (!error) {
            GameSource source;

            source.root = path;
            source.storage = path;
            source.edition = GameEdition::Remastered;
            source.format = SourceFormat::Casc;

            return source;
        }

        error.clear();
    }

    /*
     * Check for a directly available Brood War data archive
     *
     * We search case-insensitively because filenames differ between releases/filesystems:
     *
     *   broodat.mpq
     *   BrooDat.mpq
     */
    if (const auto archive = findFile(path, { "broodat.mpq" })) {
        GameSource source;

        source.root = path;
        source.storage = *archive;
        source.edition = GameEdition::BroodWar;
        source.format = SourceFormat::Mpq;

        return source;
    }

    /*
     * Check for a directly available original StarCraft data archive
     */
    if (const auto archive = findFile(path, { "stardat.mpq" })) {
        GameSource source;

        source.root = path;
        source.storage = *archive;
        source.edition = GameEdition::Classic;
        source.format = SourceFormat::Mpq;

        return source;
    }

    /*
     * Legacy StarCraft distributions appeared under several archive filenames
     */
    if (const auto archive = findFile(path, { "install.exe", "starcraft.mpq", "installer tome.mpq", "starcraft archive" })) {
        GameSource source;

        source.root = path;
        source.storage = *archive;
        source.edition = GameEdition::Unknown;
        source.format = SourceFormat::Mpq;

        return source;
    }

    return std::nullopt;
}

std::optional<std::filesystem::path> SourceDetector::findFile(const std::filesystem::path &directory, const std::vector<std::string> &names) const
{
    std::error_code error;

    std::filesystem::directory_iterator iterator(
        directory,
        error
    );

    if (error) {
        return std::nullopt;
    }

    for (const auto &entry : iterator) {
        if (!entry.is_regular_file(error)) {
            error.clear();
            continue;
        }

        const std::string filename = lowercase(entry.path().filename().string());

        for (const std::string &candidate : names) {
            if (filename == lowercase(candidate)) {
                return entry.path();
            }
        }
    }

    return std::nullopt;
}

std::string SourceDetector::lowercase(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) {
            return static_cast<char>(
                std::tolower(character)
            );
        }
    );

    return value;
}
