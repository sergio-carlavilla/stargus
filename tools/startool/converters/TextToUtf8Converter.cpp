// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "TextToUtf8Converter.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

bool TextToUtf8Converter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    std::string &error
) const
{
    std::ifstream inputFile(
        input,
        std::ios::binary
    );

    if (!inputFile) {
        error =
            "Could not open ISO-8859-1 text input: " +
            input.string();

        return false;
    }

    std::vector<std::uint8_t> data{
        std::istreambuf_iterator<char>(inputFile),
        std::istreambuf_iterator<char>()
    };

    if (!inputFile.eof() && inputFile.fail()) {
        error =
            "Could not read ISO-8859-1 text input: " +
            input.string();

        return false;
    }

    // Startool 3 used strlen() on the extracted buffer. That is unsafe when
    // the extracted resource is not NUL-terminated and can read past the end
    // of the resource. Startool 4 keeps the intended text semantics without
    // reproducing that undefined behaviour: if an embedded NUL exists, treat
    // it as the logical end of the text; otherwise convert the complete file.
    const auto terminator = std::find(
        data.begin(),
        data.end(),
        static_cast<std::uint8_t>(0)
    );

    const auto textEnd =
        terminator == data.end()
            ? data.end()
            : terminator;

    if (data.begin() == textEnd) {
        error =
            "ISO-8859-1 text input is empty: " +
            input.string();

        return false;
    }

    std::string utf8;

    utf8.reserve(
        static_cast<std::size_t>(
            std::distance(
                data.begin(),
                textEnd
            )
        ) * 2
    );

    for (
        auto iterator = data.begin();
        iterator != textEnd;
        ++iterator
    ) {
        const std::uint8_t byte = *iterator;

        if (byte < 0x80) {
            utf8.push_back(
                static_cast<char>(byte)
            );
        } else {
            utf8.push_back(
                static_cast<char>(
                    0xC0 | (byte >> 6)
                )
            );

            utf8.push_back(
                static_cast<char>(
                    0x80 | (byte & 0x3F)
                )
            );
        }
    }

    std::error_code filesystemError;

    if (std::filesystem::exists(output, filesystemError)) {
        if (filesystemError) {
            error =
                "Could not inspect UTF-8 text output '" +
                output.string() +
                "': " +
                filesystemError.message();

            return false;
        }

        error =
            "UTF-8 text output already exists: " +
            output.string();

        return false;
    }

    const std::filesystem::path parent =
        output.parent_path();

    if (!parent.empty()) {
        std::filesystem::create_directories(
            parent,
            filesystemError
        );

        if (filesystemError) {
            error =
                "Could not create UTF-8 text output directory: " +
                filesystemError.message();

            return false;
        }
    }

    std::ofstream outputFile(
        output,
        std::ios::binary
    );

    if (!outputFile) {
        error =
            "Could not open UTF-8 text output for writing: " +
            output.string();

        return false;
    }

    outputFile.write(
        utf8.data(),
        static_cast<std::streamsize>(
            utf8.size()
        )
    );

    if (!outputFile) {
        outputFile.close();

        std::error_code cleanupError;

        std::filesystem::remove(
            output,
            cleanupError
        );

        error =
            "Could not write UTF-8 text output: " +
            output.string();

        return false;
    }

    outputFile.close();

    return true;
}
