// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "UiConsoleConverter.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace
{
    constexpr int ConsoleWidth = 640;
    constexpr int ConsoleHeight = 480;

    std::string quoted(const std::filesystem::path &path)
    {
        return "\"" + path.string() + "\"";
    }

    void removeIfPresent(const std::filesystem::path &path)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
}

bool UiConsoleConverter::convert(
    const std::filesystem::path &inputPng,
    const std::filesystem::path &outputBase,
    int left,
    int right,
    std::string &error
) const
{
    if (left < 0 || right < 0 || left >= right || right > ConsoleWidth) {
        error =
            "Invalid UI console crop range: left=" +
            std::to_string(left) +
            ", right=" +
            std::to_string(right);
        return false;
    }

    std::error_code filesystemError;

    if (!std::filesystem::is_regular_file(inputPng, filesystemError)) {
        if (filesystemError) {
            error = "Could not inspect UI console input PNG: " + filesystemError.message();
        } else {
            error = "UI console input PNG does not exist: " + inputPng.string();
        }

        return false;
    }

    std::filesystem::path leftFile = outputBase;
    leftFile += "_left.png";

    std::filesystem::path rightFile = outputBase;
    rightFile += "_right.png";

    std::filesystem::path middleFile = outputBase;
    middleFile += "_middle.png";

    std::filesystem::path temporaryFile = outputBase;
    temporaryFile += "_tmp.png";

    const std::filesystem::path outputs[] = {
        leftFile,
        middleFile,
        rightFile,
        temporaryFile
    };

    for (const auto &output : outputs) {
        filesystemError.clear();

        if (std::filesystem::exists(output, filesystemError)) {
            if (filesystemError) {
                error = "Could not inspect UI console output: " + filesystemError.message();
            } else {
                error = "UI console output already exists: " + output.string();
            }

            return false;
        }
    }

    const std::filesystem::path parentDirectory = outputBase.parent_path();

    if (!parentDirectory.empty()) {
        std::filesystem::create_directories(parentDirectory, filesystemError);

        if (filesystemError) {
            error = "Could not create UI console output directory: " + filesystemError.message();
            return false;
        }
    }

    const std::string geometry =
        std::to_string(ConsoleWidth) +
        "x" +
        std::to_string(ConsoleHeight);

    const int leftAbsolute = ConsoleWidth - left;

    const std::string convertLeft =
        "convert " +
        quoted(inputPng) +
        " -crop " +
        geometry +
        "-" +
        std::to_string(leftAbsolute) +
        "+0 " +
        quoted(leftFile);

    const std::string convertRight =
        "convert " +
        quoted(inputPng) +
        " -crop " +
        geometry +
        "+" +
        std::to_string(right) +
        "+0 " +
        quoted(rightFile);

    if (!callConvert(convertLeft)) {
        removeIfPresent(leftFile);
        error = "Could not generate UI console left segment";
        return false;
    }

    if (!callConvert(convertRight)) {
        removeIfPresent(leftFile);
        removeIfPresent(rightFile);
        error = "Could not generate UI console right segment";
        return false;
    }

    const int middle = right - left;
    const int leftTemporary = leftAbsolute - middle;
    const int middleAbsolute = right - middle;

    const std::string convertTemporary =
        "convert " +
        quoted(inputPng) +
        " -crop " +
        geometry +
        "-" +
        std::to_string(leftTemporary) +
        "+0 " +
        quoted(temporaryFile);

    const std::string convertMiddle =
        "convert " +
        quoted(temporaryFile) +
        " -crop " +
        geometry +
        "+" +
        std::to_string(middleAbsolute) +
        "+0 " +
        quoted(middleFile);

    if (!callConvert(convertTemporary)) {
        removeIfPresent(leftFile);
        removeIfPresent(rightFile);
        removeIfPresent(temporaryFile);
        error = "Could not generate temporary UI console segment";
        return false;
    }

    if (!callConvert(convertMiddle)) {
        removeIfPresent(leftFile);
        removeIfPresent(rightFile);
        removeIfPresent(middleFile);
        removeIfPresent(temporaryFile);
        error = "Could not generate UI console middle segment";
        return false;
    }

    removeIfPresent(temporaryFile);

    return true;
}

bool UiConsoleConverter::callConvert(const std::string &command) const
{
    const std::string imageMagick7 = "magick " + command;

    if (std::system(imageMagick7.c_str()) == 0) {
        return true;
    }

    if (std::system(command.c_str()) == 0) {
        return true;
    }

    const std::string graphicsMagick = "gm " + command;

    return std::system(graphicsMagick.c_str()) == 0;
}
