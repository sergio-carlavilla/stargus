// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "FontDecoder.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

namespace
{

    constexpr std::size_t FontHeaderSize = 8;
    constexpr std::size_t FontLetterHeaderSize = 4;

    std::uint32_t readLittleEndian32(const std::vector<std::uint8_t> &data, std::size_t offset)
    {
        return
            static_cast<std::uint32_t>(data[offset])                |
            (static_cast<std::uint32_t>(data[offset + 1]) << 8)     |
            (static_cast<std::uint32_t>(data[offset + 2]) << 16)    |
            (static_cast<std::uint32_t>(data[offset + 3]) << 24);
    }

}

bool FontDecoder::decode(
    const std::filesystem::path &input,
    FontImage &image,
    std::string &error
) const
{
    std::ifstream inputFile(
        input,
        std::ios::binary
    );

    if (!inputFile) {
        error = "Could not open font input: " + input.string();
        return false;
    }

    const std::vector<char> inputBytes( (std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    const std::vector<std::uint8_t> data(inputBytes.begin(), inputBytes.end());

    if (!inputFile.eof() && inputFile.fail()) {
        error = "Could not read font input: " + input.string();
        return false;
    }

    if (data.size() < FontHeaderSize) {
        error = "Font input is smaller than the FONT header: " + input.string();
        return false;
    }

    if (
        data[0] != static_cast<std::uint8_t>('F') ||
        data[1] != static_cast<std::uint8_t>('O') ||
        data[2] != static_cast<std::uint8_t>('N') ||
        data[3] != static_cast<std::uint8_t>('T')
    ) {
        error = "Font input does not contain a FONT header: " + input.string();
        return false;
    }

    FontImage decoded;
    decoded.lowIndex = data[4];
    decoded.highIndex = data[5];
    decoded.maxWidth = data[6];
    decoded.maxHeight = data[7];

    if (decoded.highIndex < decoded.lowIndex) {
        error = "Font input has an invalid character index range: " + input.string();
        return false;
    }

    // Startool legacy deliberately uses highIndex - lowIndex, without +1.
    const std::size_t letterCount = static_cast<std::size_t>( decoded.highIndex - decoded.lowIndex);

    decoded.width = static_cast<std::size_t>(decoded.maxWidth);

    if (decoded.width != 0 && letterCount > std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(decoded.maxHeight)) {
        error = "Font output dimensions are too large: " + input.string();
        return false;
    }

    decoded.height = letterCount * static_cast<std::size_t>(decoded.maxHeight);
    if (decoded.width == 0 || decoded.height == 0) {
        error = "Font input has empty output dimensions: " + input.string();
        return false;
    }

    if (letterCount > (std::numeric_limits<std::size_t>::max() - FontHeaderSize) / sizeof(std::uint32_t)) {
        error = "Font offset table is too large: " + input.string();
        return false;
    }

    const std::size_t offsetsEnd = FontHeaderSize + letterCount * sizeof(std::uint32_t);
    if (offsetsEnd > data.size()) {
        error = "Font input has an incomplete offset table: " + input.string();
        return false;
    }

    if (decoded.height > std::numeric_limits<std::size_t>::max() / decoded.width) {
        error = "Font output image is too large: " + input.string();
        return false;
    }

    decoded.pixels.assign( decoded.width * decoded.height, 255);

    std::vector<std::uint32_t> offsets(letterCount);

    for (std::size_t letterIndex = 0; letterIndex < letterCount; ++letterIndex) {
        offsets[letterIndex] = readLittleEndian32(data, FontHeaderSize + letterIndex * sizeof(std::uint32_t));
    }

    for (std::size_t letterIndex = 0; letterIndex < letterCount; ++letterIndex) {
        const std::uint32_t rawOffset = offsets[letterIndex];

        if (rawOffset == 0) {
            continue;
        }

        const std::size_t offset = static_cast<std::size_t>(rawOffset);

        if (offset > data.size() || data.size() - offset < FontLetterHeaderSize) {
            error = "Font letter header offset is outside the input: " + input.string();
            return false;
        }

        const std::uint8_t letterWidth = data[offset];
        const std::uint8_t letterHeight = data[offset + 1];
        const std::uint8_t xOffset = data[offset + 2];
        const std::uint8_t yOffset = data[offset + 3];

        if (letterWidth == 0 || letterHeight == 0) {
            continue;
        }

        std::size_t position = offset + FontLetterHeaderSize;

        std::size_t x = 0;
        std::size_t y = 0;

        for (;;) {
            if (position >= data.size()) {
                error = "Font letter data ends before the glyph is complete: " + input.string();
                return false;
            }

            const std::uint8_t control = data[position++];

            // Preserve the Startool legacy decoding algorithm exactly:
            // upper five bits skip transparent pixels, lower three bits
            // are the palette index written after the skip.
            x += static_cast<std::size_t>((control >> 3) & 0x1F);

            if (x >= static_cast<std::size_t>(letterWidth)) {
                x -= static_cast<std::size_t>(letterWidth);
                ++y;

                if (y >= static_cast<std::size_t>(letterHeight)) {
                    break;
                }
            }

            // Match Startool legacy exactly. The legacy decoder treats dp as a
            // linear pointer and does not require x to be smaller than the
            // glyph width after a single wrap. Some fonts rely on that
            // behaviour.
            const std::size_t cellBase = letterIndex * decoded.width * static_cast<std::size_t>(decoded.maxHeight);
            const std::size_t glyphBase = cellBase + static_cast<std::size_t>(xOffset) + static_cast<std::size_t>(yOffset) * decoded.width;
            const std::size_t destination = glyphBase + y * decoded.width + x;

            if (destination >= decoded.pixels.size()) {
                error = "Font letter data writes outside the output atlas: " + input.string();
                return false;
            }

            decoded.pixels[destination] = control & 0x07;
            ++x;

            if (x >= static_cast<std::size_t>(letterWidth)) {
                x -= static_cast<std::size_t>(letterWidth);
                ++y;

                if (y >= static_cast<std::size_t>(letterHeight)) {
                    break;
                }
            }
        }
    }

    image = std::move(decoded);

    return true;
}
