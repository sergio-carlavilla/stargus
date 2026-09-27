// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PortraitLuaWriter.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    std::string quote(const std::string &value)
    {
        std::string result;
        result.reserve(value.size() + 2);
        result += '"';

        for (const char character : value) {
            if (character == '\\' || character == '"') {
                result += '\\';
            }

            result += character;
        }

        result += '"';
        return result;
    }

    bool prepareOutput(
        const std::filesystem::path &output,
        std::string &error
    )
    {
        std::error_code filesystemError;

        if (std::filesystem::exists(output, filesystemError)) {
            if (filesystemError) {
                error =
                    "Could not inspect portrait Lua output '" +
                    output.string() +
                    "': " +
                    filesystemError.message();
            } else {
                error = "Portrait Lua output already exists: " + output.string();
            }

            return false;
        }

        const std::filesystem::path directory = output.parent_path();

        if (!directory.empty()) {
            std::filesystem::create_directories(directory, filesystemError);

            if (filesystemError) {
                error =
                    "Could not create portrait Lua output directory: " +
                    filesystemError.message();

                return false;
            }
        }

        return true;
    }
}

bool PortraitLuaWriter::writePortrait(
    const std::string &id,
    const std::vector<std::string> &assets,
    const std::filesystem::path &output,
    std::string &error
) const
{
    if (!prepareOutput(output, error)) {
        return false;
    }

    std::ofstream file(output);

    if (!file) {
        error = "Could not open portrait Lua output for writing: " + output.string();
        return false;
    }

    file << "portrait_" << id << " = {";

    for (std::size_t index = 0; index < assets.size(); ++index) {
        if (index != 0) {
            file << ", ";
        }

        file << quote(assets[index]);
    }

    file << "}";

    if (!file) {
        file.close();

        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);

        error = "Could not write portrait Lua output: " + output.string();
        return false;
    }

    file.close();
    return true;
}

bool PortraitLuaWriter::writeLoader(
    const std::vector<std::string> &portraitLuaFiles,
    const std::filesystem::path &output,
    std::string &error
) const
{
    if (!prepareOutput(output, error)) {
        return false;
    }

    std::ofstream file(output);

    if (!file) {
        error = "Could not open portrait Lua loader for writing: " + output.string();
        return false;
    }

    for (const std::string &portraitLuaFile : portraitLuaFiles) {
        file << "Load(" << quote(portraitLuaFile) << ")\n";
    }

    if (!file) {
        file.close();

        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);

        error = "Could not write portrait Lua loader: " + output.string();
        return false;
    }

    file.close();
    return true;
}
