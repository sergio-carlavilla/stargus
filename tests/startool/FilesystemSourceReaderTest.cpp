// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "FilesystemSourceReader.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>

namespace
{
    struct TemporaryDirectory
    {
        std::filesystem::path path;

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    };

    bool writeText(const std::filesystem::path &path, const std::string &value)
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
}

int main()
{
    std::error_code error;
    const auto tempRoot = std::filesystem::temp_directory_path(error);
    if (error) {
        std::cerr << "Could not find test temporary directory\n";
        return 1;
    }

    TemporaryDirectory temp{
        tempRoot / (
            "startool-filesystem-source-reader-test-" +
            std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()
            )
        )
    };

    const auto root = temp.path / "installation";
    const auto maps = root / "Maps" / "campaign";
    std::filesystem::create_directories(maps, error);
    if (error || !writeText(maps / "(1)Enslavers01.scm", "scenario data")) {
        std::cerr << "Could not create test map\n";
        return 1;
    }

    LogicalSource logicalSource{
        "files", SourceFormat::Directory, SourceLocation::Filesystem, root, {}
    };
    FilesystemSourceReader reader(logicalSource);

    const std::string fileName = "Maps/campaign/(1)Enslavers01.scm";
    const auto output = temp.path / "extracted.scm";

    if (!reader.isOpen() || !reader.contains(fileName) ||
        !reader.extract(fileName, output) || readText(output) != "scenario data") {
        std::cerr << "Could not extract a map in the installation\n";
        return 1;
    }

    if (reader.contains("Maps/missing.scm") ||
        reader.contains("../outside.scm") ||
        reader.contains("Maps/../Maps/campaign/(1)Enslavers01.scm") ||
        reader.contains("/etc/passwd") ||
        reader.contains("C:\\Windows\\win.ini") ||
        reader.extract("../outside.scm", temp.path / "bad.scm")) {
        std::cerr << "Invalid or escaping path was accepted\n";
        return 1;
    }

    // A symlink from Maps/ to a file outside the root must not escape the sandbox.
    const auto external = temp.path / "outside.scm";
    if (!writeText(external, "outside")) {
        return 1;
    }

    std::filesystem::create_symlink(external, maps / "escape.scm", error);
    if (!error) {
        if (reader.contains("Maps/campaign/escape.scm") ||
            reader.extract("Maps/campaign/escape.scm", temp.path / "escaped.scm")) {
            std::cerr << "Symlink escaped installation root\n";
            return 1;
        }
    }

    LogicalSource invalidSource{
        "files", SourceFormat::Mpq, SourceLocation::Filesystem, root, {}
    };
    if (FilesystemSourceReader(invalidSource).isOpen()) {
        std::cerr << "Reader opened a non-directory source format\n";
        return 1;
    }

    return 0;
}
