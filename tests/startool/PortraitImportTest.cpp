// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

/*
 * Runs startool4 on a synthetic CD installer whose portraits are WebM, the
 * way the Battle.net CASC release stores them, and checks that the portrait
 * Lua references the converted MNG files instead of an empty sequence.
 *
 * No Blizzard data is used: portdata.dat/tbl are written here and the WebM
 * videos are generated with FFmpeg. The test is skipped (exit 77) when
 * FFmpeg cannot encode VP9.
 */

#include <StormLib.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
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
    constexpr int SkipReturnCode = 77;

    struct TemporaryDirectory
    {
        std::filesystem::path path;

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    };

    struct MngInfo
    {
        bool valid = false;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        int images = 0;
    };

    std::string quote(const std::filesystem::path &path)
    {
        return "\"" + path.string() + "\"";
    }

    bool writeBinary(const std::filesystem::path &path, const std::string &value)
    {
        std::ofstream file(path, std::ios::binary);
        file << value;
        return static_cast<bool>(file);
    }

    std::string readText(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::string(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
    }

    void appendLe16(std::string &data, std::uint16_t value)
    {
        data += static_cast<char>(value & 0xff);
        data += static_cast<char>((value >> 8) & 0xff);
    }

    void appendLe32(std::string &data, std::uint32_t value)
    {
        appendLe16(data, static_cast<std::uint16_t>(value & 0xffff));
        appendLe16(data, static_cast<std::uint16_t>(value >> 16));
    }

    std::uint32_t readBe32(const std::string &data, std::size_t offset)
    {
        return
            (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset])) << 24) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 1])) << 16) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 2])) << 8) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 3]));
    }

    // One portrait: idle sequence tfake\TFkFid0*, talking sequence tfake\TFkTlk0*
    std::string portdataTbl()
    {
        const std::vector<std::string> entries = { "tfake\\TFkFid0", "tfake\\TFkTlk0" };

        std::string data;
        appendLe16(data, static_cast<std::uint16_t>(entries.size()));

        std::uint16_t offset = static_cast<std::uint16_t>(2 + entries.size() * 2);
        for (const std::string &entry : entries) {
            appendLe16(data, offset);
            offset = static_cast<std::uint16_t>(offset + entry.size() + 1);
        }

        for (const std::string &entry : entries) {
            data += entry;
            data += '\0';
        }

        return data;
    }

    std::string portdataDat()
    {
        std::string data;
        appendLe32(data, 1);
        appendLe32(data, 2);
        data += std::string(4, '\0');
        return data;
    }

    bool makeWebm(const std::filesystem::path &output, int frames)
    {
        const std::string command =
            "ffmpeg -v error -y -f lavfi -i color=c=red:s=8x8:r=10 -frames:v " +
            std::to_string(frames) +
            " -codec:v libvpx-vp9 " +
            quote(output);

        return std::system(command.c_str()) == 0;
    }

    bool createMpq(
        const std::filesystem::path &path,
        const std::vector<std::pair<std::string, std::filesystem::path>> &files
    )
    {
        HANDLE archive = nullptr;
        const std::string archiveName = path.string();

        if (!SFileCreateArchive(archiveName.c_str(), MPQ_CREATE_ARCHIVE_V1, 16, &archive)) {
            return false;
        }

        bool added = true;

        for (const auto &[archivedName, source] : files) {
            const std::string sourceName = source.string();
            added = added && SFileAddFileEx(
                archive,
                sourceName.c_str(),
                archivedName.c_str(),
                MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING,
                MPQ_COMPRESSION_ZLIB,
                MPQ_COMPRESSION_NEXT_SAME
            );
        }

        return SFileCloseArchive(archive) && added;
    }

    /*
     * Builds <root>/install.exe containing files\stardat.mpq, the layout of
     * the original CD that MpqVerifier accepts
     */
    bool createInstallation(
        const std::filesystem::path &root,
        const std::filesystem::path &work,
        const std::vector<std::pair<std::string, int>> &videos
    )
    {
        std::error_code error;
        std::filesystem::create_directories(root, error);
        std::filesystem::create_directories(work, error);

        if (error) {
            return false;
        }

        const std::filesystem::path dat = work / "portdata.dat";
        const std::filesystem::path tbl = work / "portdata.tbl";

        if (!writeBinary(dat, portdataDat()) || !writeBinary(tbl, portdataTbl())) {
            return false;
        }

        std::vector<std::pair<std::string, std::filesystem::path>> files = {
            { "arr\\portdata.dat", dat },
            { "arr\\portdata.tbl", tbl },
        };

        for (const auto &[name, frames] : videos) {
            const std::filesystem::path video = work / (name + ".webm");

            if (!makeWebm(video, frames)) {
                return false;
            }

            files.emplace_back("portrait\\tfake\\" + name + ".webm", video);
        }

        const std::filesystem::path stardat = work / "stardat.mpq";

        return createMpq(stardat, files) &&
            createMpq(root / "install.exe", { { "files\\stardat.mpq", stardat } });
    }

    MngInfo readMng(const std::filesystem::path &path)
    {
        const std::string data = readText(path);
        MngInfo info;

        if (data.size() < 8 || data.compare(0, 8, "\x8aMNG\r\n\x1a\n") != 0) {
            return info;
        }

        for (std::size_t offset = 8; offset + 12 <= data.size();) {
            const std::uint32_t length = readBe32(data, offset);
            const std::string type = data.substr(offset + 4, 4);

            if (type == "MHDR" && length >= 8) {
                info.width = readBe32(data, offset + 8);
                info.height = readBe32(data, offset + 12);
            } else if (type == "IHDR") {
                ++info.images;
            }

            offset += 12 + length;
        }

        info.valid = true;
        return info;
    }

    int runImport(
        const std::filesystem::path &startool,
        const std::filesystem::path &installation,
        const std::filesystem::path &manifest,
        const std::filesystem::path &destination,
        const std::filesystem::path &log
    )
    {
        const std::string command =
            quote(startool) + " import " +
            quote(installation) + " " +
            quote(manifest) + " " +
            quote(destination) + " > " +
            quote(log) + " 2>&1";

        return std::system(command.c_str());
    }

    bool expectMng(const std::filesystem::path &path, int images)
    {
        const MngInfo info = readMng(path);

        if (!info.valid || info.width != 8 || info.height != 8 || info.images != images) {
            std::cerr
                << "Unexpected MNG " << path
                << ": valid=" << info.valid
                << " size=" << info.width << "x" << info.height
                << " images=" << info.images
                << " (expected 8x8, " << images << " images)\n";
            return false;
        }

        return true;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Usage: PortraitImportTest <startool4> <classic/portraits.json>\n";
        return 1;
    }

    const std::filesystem::path startool = argv[1];
    const std::filesystem::path portraitRules = argv[2];

    if (std::system("ffmpeg -v error -f lavfi -i color=c=red:s=8x8:r=10 "
                    "-frames:v 1 -codec:v libvpx-vp9 -f null -") != 0) {
        std::cout << "SKIP: FFmpeg with the libvpx-vp9 encoder is required\n";
        return SkipReturnCode;
    }

    std::error_code error;
    const auto tempRoot = std::filesystem::temp_directory_path(error);
    if (error) {
        std::cerr << "Could not find test temporary directory\n";
        return 1;
    }

    TemporaryDirectory temp{
        tempRoot / (
            "startool-portrait-import-test-" +
            std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()
            )
        )
    };

    // Use the real portrait rule through a wrapper manifest
    const std::filesystem::path manifest = temp.path / "manifest" / "portraits-only.json";
    std::filesystem::create_directories(manifest.parent_path() / "classic", error);
    std::filesystem::copy_file(portraitRules, manifest.parent_path() / "classic" / "portraits.json", error);

    if (error || !writeBinary(manifest, "{\"format_version\": 1, \"includes\": [\"classic/portraits.json\"]}\n")) {
        std::cerr << "Could not create test manifest\n";
        return 1;
    }

    // WebM portraits are converted and referenced like SMK portraits
    {
        const std::filesystem::path installation = temp.path / "webm" / "cd";
        const std::filesystem::path output = temp.path / "webm" / "out";
        const std::filesystem::path log = temp.path / "webm" / "import.log";

        if (!createInstallation(installation, temp.path / "webm" / "work", { { "TFkFid00", 3 }, { "TFkFid01", 2 }, { "TFkTlk00", 4 } })) {
            std::cerr << "Could not create the WebM test installation\n";
            return 1;
        }

        if (runImport(startool, installation, manifest, output, log) != 0) {
            std::cerr << "Import of WebM portraits failed:\n" << readText(log);
            return 1;
        }

        const std::string lua = readText(output / "luagen/portrait/portrait-tfake.lua");
        const std::string expected =
            "portrait_tfake = {"
            "\"videos/portrait/tfake/tfkfid00.mng\", "
            "\"videos/portrait/tfake/tfkfid01.mng\", "
            "\"talking\", "
            "\"videos/portrait/tfake/tfktlk00.mng\"}";

        if (lua.find(expected) == std::string::npos) {
            std::cerr << "Unexpected portrait Lua:\n" << lua << '\n';
            return 1;
        }

        if (!expectMng(output / "videos/portrait/tfake/tfkfid00.mng", 3) ||
            !expectMng(output / "videos/portrait/tfake/tfkfid01.mng", 2) ||
            !expectMng(output / "videos/portrait/tfake/tfktlk00.mng", 4)) {
            return 1;
        }
    }

    // A sequence whose first video is missing fails instead of writing {"talking"}
    {
        const std::filesystem::path installation = temp.path / "missing" / "cd";
        const std::filesystem::path output = temp.path / "missing" / "out";
        const std::filesystem::path log = temp.path / "missing" / "import.log";

        if (!createInstallation(installation, temp.path / "missing" / "work", { { "TFkFid00", 3 } })) {
            std::cerr << "Could not create the incomplete test installation\n";
            return 1;
        }

        if (runImport(startool, installation, manifest, output, log) == 0) {
            std::cerr << "Import without a talking portrait video should fail\n";
            return 1;
        }

        if (readText(log).find("portrait/tfake/TFkTlk00' was not found as .smk or .webm") == std::string::npos) {
            std::cerr << "Missing portrait error not reported:\n" << readText(log);
            return 1;
        }

        if (std::filesystem::exists(output / "luagen/portrait/portrait-tfake.lua", error)) {
            std::cerr << "Portrait Lua must not be written for an incomplete portrait\n";
            return 1;
        }
    }

    std::cout << "PortraitImportTest passed\n";
    return 0;
}
