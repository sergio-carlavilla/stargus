// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SourceDetector.h"
#include "MpqVerifier.h"
#include "MpqArchive.h"
#include "MpqSourceReader.h"

#include <StormLib.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
    struct TemporaryDirectory
    {
        std::filesystem::path path;
        bool remove = true;
        ~TemporaryDirectory()
        {
            if (remove) {
                std::error_code error;
                std::filesystem::remove_all(path, error);
            }
        }
    };

    bool writeFile(const std::filesystem::path &path, const std::string &text)
    {
        std::ofstream stream(path, std::ios::binary);
        stream << text;
        return static_cast<bool>(stream);
    }

    std::string readFile(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }

    bool createMpq(
        const std::filesystem::path &path,
        const std::vector<std::pair<std::string, std::filesystem::path>> &files
    )
    {
        std::filesystem::create_directories(path.parent_path());
        HANDLE archive = nullptr;
        const std::string name = path.string();
        if (!SFileCreateArchive(name.c_str(), MPQ_CREATE_ARCHIVE_V1, 32, &archive)) {
            return false;
        }
        bool passed = true;
        for (const auto &[member, input] : files) {
            const std::string source = input.string();
            if (!SFileAddFileEx(archive, source.c_str(), member.c_str(),
                    MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING,
                    MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME)) {
                passed = false;
                break;
            }
        }
        return SFileCloseArchive(archive) && passed;
    }

    const LogicalSource *find(const GameSource &source, const std::string &id)
    {
        for (const auto &item : source.sources) {
            if (item.id == id) {
                return &item;
            }
        }
        return nullptr;
    }

    bool checkSource(
        const std::filesystem::path &path,
        GameEdition edition,
        SourceLocation location,
        bool installerExpected,
        const std::string &payload,
        const std::filesystem::path &output
    )
    {
        auto source = SourceDetector{}.detect(path);
        if (!source || !MpqVerifier{}.verify(*source) || source->edition != edition) {
            std::cerr << "Could not detect/verify " << path << '\n';
            return false;
        }
        const auto *game = find(*source, "game");
        if (game == nullptr || game->location != location ||
            (find(*source, "installer") != nullptr) != installerExpected ||
            source->sources.size() != (installerExpected ? 2U : 1U)) {
            std::cerr << "Incorrect source mapping for " << path << '\n';
            return false;
        }
        MpqSourceReader reader(*game);
        if (!reader.isOpen() || !reader.contains("arr/units.dat") ||
            !reader.extract("arr/units.dat", output) || readFile(output) != payload) {
            std::cerr << "Could not read game payload from " << path << '\n';
            return false;
        }
        if (installerExpected) {
            MpqSourceReader installer(*find(*source, "installer"));
            const auto font = output.string() + "-font";
            if (!installer.extract("files/font/fixture.txt", font) || readFile(font) != "installer-payload") {
                std::cerr << "Incorrect installer payload for " << path << '\n';
                return false;
            }
        }
        if (!MpqVerifier{}.verify(*source) ||
            source->sources.size() != (installerExpected ? 2U : 1U)) {
            std::cerr << "Repeated verification appended sources\n";
            return false;
        }
        return true;
    }

    int run(const std::filesystem::path &root)
    {
        const auto payload = root / "base.txt";
        const auto expansion = root / "expansion.txt";
        const auto font = root / "font.txt";
        if (!writeFile(payload, "base-payload") || !writeFile(expansion, "expansion-payload") ||
            !writeFile(font, "installer-payload")) {
            return 1;
        }
        const auto classic = root / "classic" / "StarDat.mpq";
        const auto broodwar = root / "broodwar" / "BrooDat.mpq";
        if (!createMpq(classic, {{"arr\\units.dat", payload}, {"shared.txt", payload}, {"base-only.txt", payload}}) ||
            !createMpq(broodwar, {{"arr\\units.dat", expansion}, {"shared.txt", expansion}, {"expansion-only.txt", expansion}})) {
            std::cerr << "Could not create raw game MPQs\n";
            return 1;
        }
        if (!checkSource(classic, GameEdition::Classic, SourceLocation::Filesystem,
                false, "base-payload", root / "extracted-classic") ||
            !checkSource(classic.parent_path(), GameEdition::Classic, SourceLocation::Filesystem,
                false, "base-payload", root / "extracted-classic-directory") ||
            !checkSource(broodwar, GameEdition::BroodWar, SourceLocation::Filesystem,
                false, "expansion-payload", root / "extracted-broodwar") ||
            !checkSource(broodwar.parent_path(), GameEdition::BroodWar, SourceLocation::Filesystem,
                false, "expansion-payload", root / "extracted-broodwar-directory")) {
            return 1;
        }
        std::cout << "PASS: mixed-case raw Classic/Brood War, file and directory inputs\n";

        const auto installed = root / "installed";
        std::filesystem::create_directory(installed);
        std::filesystem::copy_file(classic, installed / "StarDat.mpq");
        std::filesystem::copy_file(broodwar, installed / "BrooDat.mpq");
        if (!checkSource(installed, GameEdition::BroodWar, SourceLocation::Filesystem,
                false, "expansion-payload", root / "extracted-installed") ||
            !checkSource(installed / "StarDat.mpq", GameEdition::Classic, SourceLocation::Filesystem,
                false, "base-payload", root / "extracted-installed-base")) {
            return 1;
        }
        std::cout << "PASS: both MPQs in one directory with an explicit Classic base\n";

        const auto classicCd = root / "classic-cd" / "install.exe";
        const auto broodwarCd = root / "broodwar-cd" / "INSTALL.EXE";
        if (!createMpq(classicCd, {{"files\\stardat.mpq", classic}, {"files\\font\\fixture.txt", font}}) ||
            !createMpq(broodwarCd, {{"files\\broodat.mpq", broodwar}, {"files\\font\\fixture.txt", font}}) ||
            !checkSource(classicCd.parent_path(), GameEdition::Classic, SourceLocation::ArchiveMember,
                true, "base-payload", root / "extracted-classic-cd") ||
            !checkSource(broodwarCd.parent_path(), GameEdition::BroodWar, SourceLocation::ArchiveMember,
                true, "expansion-payload", root / "extracted-broodwar-cd")) {
            return 1;
        }
        std::cout << "PASS: existing nested Classic/Brood War CD layouts\n";

        const auto companion = root / "companion";
        std::filesystem::create_directory(companion);
        std::filesystem::copy_file(classic, companion / "STARDAT.MPQ");
        std::filesystem::copy_file(classicCd, companion / "INSTALL.EXE");
        if (!checkSource(companion, GameEdition::Classic, SourceLocation::Filesystem,
                true, "base-payload", root / "extracted-companion")) {
            return 1;
        }
        const auto wrong = root / "wrong-companion";
        std::filesystem::create_directory(wrong);
        std::filesystem::copy_file(classic, wrong / "stardat.mpq");
        std::filesystem::copy_file(broodwarCd, wrong / "install.exe");
        if (!checkSource(wrong, GameEdition::Classic, SourceLocation::Filesystem,
                false, "base-payload", root / "extracted-wrong-companion")) {
            return 1;
        }
        std::cout << "PASS: matching companion installer exposed; other edition ignored\n";

        const auto unrelated = root / "unrelated" / "stardat.mpq";
        const auto unknown = root / "unknown" / "data.mpq";
        if (!createMpq(unrelated, {{"unrelated.txt", payload}}) ||
            !createMpq(unknown, {{"arr\\units.dat", payload}})) {
            return 1;
        }
        const auto corrupt = root / "corrupt" / "broodat.mpq";
        std::filesystem::create_directory(corrupt.parent_path());
        if (!writeFile(corrupt, "not an MPQ")) {
            return 1;
        }
        if (!createMpq(root / "damaged-expansion-cd" / "install.exe",
                {{"files\\broodat.mpq", corrupt}})) {
            return 1;
        }
        for (const auto &path : {unrelated, unknown, corrupt}) {
            auto source = SourceDetector{}.detect(path);
            if (!source) {
                return 1;
            }
            source->sources.push_back(LogicalSource{"stale", SourceFormat::Mpq,
                SourceLocation::Filesystem, path, {}});
            if (MpqVerifier{}.verify(*source) || !source->sources.empty()) {
                std::cerr << "Accepted unrelated/corrupt archive or retained stale sources: " << path << '\n';
                return 1;
            }
        }
        std::cout << "PASS: unrelated, unknown-name and corrupt archives rejected; failure clears sources\n";
        return 0;
    }
}

int main(int argc, char **argv)
{
    if (argc > 2) {
        return 2;
    }
    std::error_code error;
    const auto temporary = std::filesystem::temp_directory_path(error);
    if (error) {
        return 1;
    }
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = argc == 2 ? std::filesystem::path(argv[1])
        : temporary / ("startool-mpq-test-" + std::to_string(stamp));
    if (!std::filesystem::create_directory(path, error) || error) {
        std::cerr << "Refusing existing/unwritable fixture directory: " << path << '\n';
        return 1;
    }
    TemporaryDirectory root{path, argc == 1};
    return run(root.path);
}
