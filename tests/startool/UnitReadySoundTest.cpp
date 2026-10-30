// SPDX-License-Identifier: GPL-2.0-only
#include "converters/UnitDataResolver.h"
#include "converters/UnitLuaWriter.h"
#include "converters/UnitsLuaWriter.h"
#include "FilesystemSourceReader.h"
#include <StormLib.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
namespace fs = std::filesystem;
using Bytes = std::vector<unsigned char>;

void require(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

void le16(Bytes &data, std::size_t offset, unsigned value)
{
    data.at(offset) = static_cast<unsigned char>(value);
    data.at(offset + 1) = static_cast<unsigned char>(value >> 8);
}

void le32(Bytes &data, std::size_t offset, unsigned value)
{
    for (unsigned i = 0; i < 4; ++i) data.at(offset + i) = static_cast<unsigned char>(value >> (i * 8));
}

void write(const fs::path &path, const Bytes &data)
{
    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    require(static_cast<bool>(stream), "Could not write " + path.string());
}

void tbl(const fs::path &path, const std::vector<std::string> &strings)
{
    Bytes data(2 + strings.size() * 2, 0);
    le16(data, 0, static_cast<unsigned>(strings.size()));
    for (std::size_t i = 0; i < strings.size(); ++i) {
        le16(data, 2 + i * 2, static_cast<unsigned>(data.size()));
        data.insert(data.end(), strings[i].begin(), strings[i].end());
        data.push_back(0);
    }
    write(path, data);
}

UnitDataFiles tables(const fs::path &root, bool broodwar)
{
    Bytes units(broodwar ? 19876 : 19192, 0);
    const std::size_t ready = broodwar ? 9312 : 8856;
    le16(units, ready, 2); // Marine's physically available creation voice.
    le16(units, ready + 74 * 2, 1); // Hero's creation voice can be absent in Classic.
    le16(units, ready + 212 + 74 * 2, 2); // Keep selection voice independently.
    le16(units, ready + 212 + 228 * 2 + 74 * 2, 2);
    le32(units, 3156, 40U << 8);
    le32(units, 3156 + 74 * 4, 80U << 8);
    write(root / "arr/units.dat", units);
    write(root / "arr/flingy.dat", Bytes(15, 0));
    write(root / "arr/sprites.dat", Bytes(520, 0));
    Bytes images(38, 0); le32(images, 0, 1);
    write(root / "arr/images.dat", images);
    tbl(root / "arr/images.tbl", {"terran\\marine.grp"});
    Bytes portraits(12, 0); le32(portraits, 0, 1); le32(portraits, 4, 1);
    write(root / "arr/portdata.dat", portraits);
    tbl(root / "arr/portdata.tbl", {"tmarine\\tmafid00.smk"});
    Bytes sounds(27, 0); le32(sounds, 4, 1); le32(sounds, 8, 2);
    write(root / "arr/sfxdata.dat", sounds);
    tbl(root / "arr/sfxdata.tbl", {"Protoss\\DarkTemplar\\PdtRdy00.WAV", "Terran\\Marine\\TmaRdy00.wav"});
    tbl(root / "rez/stat_txt.tbl", std::vector<std::string>(228, "Fixture unit"));
    write(root / "sound/terran/marine/tmardy00.wav", Bytes{'v', 'o', 'i', 'c', 'e'});
    return {root / "arr/units.dat", root / "arr/flingy.dat", root / "arr/sprites.dat",
            root / "arr/images.dat", root / "arr/images.tbl", root / "arr/portdata.dat",
            root / "arr/portdata.tbl", root / "arr/sfxdata.dat", root / "arr/sfxdata.tbl", root / "rez/stat_txt.tbl"};
}

void check(const fs::path &root, const UnitDataFiles &files)
{
    LogicalSource source;
    source.format = SourceFormat::Directory;
    source.location = SourceLocation::Filesystem;
    source.storage = root;
    FilesystemSourceReader reader(source);
    UnitDataResolver resolver(&reader);
    std::string error;
    require(resolver.load(files, error), error);
    UnitMetadata hero, marine, noVoice;
    require(resolver.resolve(74, "hero", hero, error), error);
    require(hero.readySounds.empty(), "Missing creation voice was emitted");
    require(hero.whatSounds == std::vector<std::string>{"sounds/unit/terran/marine/tmardy00.ogg"}, "Selection voice changed");
    require(hero.hitPoints == 80, "HP changed");
    require(resolver.resolve(0, "marine", marine, error), error);
    require(marine.readySounds == std::vector<std::string>{"sounds/unit/terran/marine/tmardy00.ogg"}, "Existing creation voice lost");
    require(resolver.resolve(1, "no-voice", noVoice, error), error);
    require(noVoice.readySounds.empty(), "SoundNone gained a voice");
    require(resolver.resolve(files, 74, "hero", hero, error), error);
    require(hero.readySounds.empty(), "Single-unit overload lost source context");
    const std::string absent = UnitLuaWriter{}.render(hero);
    require(absent.find("pdtrdy00") == std::string::npos && absent.find("hero-sound-ready") == std::string::npos,
            "Generated Lua retains a missing voice or sound ID");
    require(UnitsLuaWriter{}.write({{74, "hero", true}, {0, "marine", true}}, resolver, root / "lua/luagen-units.lua", error), error);
    write(root / "sound/protoss/darktemplar/pdtrdy00.wav", Bytes{'v', 'o', 'i', 'c', 'e'});
    require(resolver.resolve(74, "hero", hero, error), error);
    require(hero.readySounds == std::vector<std::string>{"sounds/unit/protoss/darktemplar/pdtrdy00.ogg"}, "Existing hero voice lost");
    hero.readySounds.clear();
    require(UnitLuaWriter{}.render(hero) == absent, "Non-ready fields changed");
    fs::remove(root / "sound/protoss/darktemplar/pdtrdy00.wav");
}

void mpq(const fs::path &path, const fs::path &root, bool withVoice)
{
    fs::create_directories(path.parent_path());
    HANDLE handle = nullptr;
    const std::string archive = path.string();
    require(SFileCreateArchive(archive.c_str(), MPQ_CREATE_ARCHIVE_V1, 32, &handle), "Could not create MPQ");
    bool ok = true;
    for (const auto &entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        std::string relative = entry.path().lexically_relative(root).generic_string();
        if (relative.rfind("lua/", 0) == 0) continue;
        std::replace(relative.begin(), relative.end(), '/', '\\');
        const std::string input = entry.path().string();
        ok = SFileAddFileEx(handle, input.c_str(), relative.c_str(), MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME) && ok;
    }
    if (withVoice) {
        const std::string input = (root / "sound/terran/marine/tmardy00.wav").string();
        ok = SFileAddFileEx(handle, input.c_str(), "sound\\protoss\\darktemplar\\pdtrdy00.wav", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME) && ok;
    }
    const bool closed = SFileCloseArchive(handle);
    require(ok && closed, "Could not populate/close MPQ");
}
} // namespace

int main(int argc, char **argv)
{
    if (argc > 2) return 2;
    const bool keep = argc == 2;
    const auto root = keep ? fs::path(argv[1]) : fs::temp_directory_path() /
        ("startool-ready-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        require(fs::create_directory(root), "Fixture directory already exists");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    struct Cleanup {
        fs::path path; bool keep;
        ~Cleanup() { if (!keep) { std::error_code error; fs::remove_all(path, error); } }
    } cleanup{root, keep};
    try {
        for (const bool broodwar : {false, true}) {
            const std::string label = broodwar ? "broodwar" : "classic";
            const auto directory = root / ("tables-" + label);
            const auto files = tables(directory, broodwar);
            check(directory, files);
            for (const bool available : {false, true}) {
                mpq(root / (label + (available ? "-present" : "-missing")) / (broodwar ? "BrooDat.mpq" : "StarDat.mpq"), directory, available);
            }
        }
        std::cout << "PASS: Classic/Brood War DATs; absent/present ready voice, normalized paths, selection/HP preserved, both resolver APIs and Lua generation\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
