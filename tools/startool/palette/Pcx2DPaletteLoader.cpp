// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "Pcx2DPaletteLoader.h"

#include "formats/PcxDecoder.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <utility>

namespace
{

    std::uint16_t readLittleEndian16(const std::array<std::uint8_t, 128> &header, std::size_t offset)
    {
        return static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(header[offset]) |
            (static_cast<std::uint16_t>(header[offset + 1]) << 8)
        );
    }

}

bool Pcx2DPaletteLoader::load(
    const std::filesystem::path &input,
    const PaletteDefinition &definition,
    Palette2D &palette,
    std::string &error
) const
{
    if (definition.mapping) {
        error = "PCX2D palette '" + definition.id + "' must not define mapping";
        return false;
    }

    PcxDecoder decoder;
    PcxImage image;

    if (!decoder.decode(input, image, error)) {
        return false;
    }

    std::ifstream inputStream(input, std::ios::binary);
    if (!inputStream) {
        error = "Could not open PCX2D palette: " + input.string();
        return false;
    }

    std::array<std::uint8_t, 128> header{};
    inputStream.read(
        reinterpret_cast<char *>(header.data()),
        static_cast<std::streamsize>(header.size())
    );

    if (inputStream.gcount() != static_cast<std::streamsize>(header.size())) {
        error = "PCX2D palette has an incomplete PCX header: " + input.string();
        return false;
    }

    const std::uint16_t xMin = readLittleEndian16(header, 4);
    const std::uint16_t yMin = readLittleEndian16(header, 6);
    const std::uint16_t xMax = readLittleEndian16(header, 8);
    const std::uint16_t yMax = readLittleEndian16(header, 10);

    if (xMax < xMin || yMax < yMin) {
        error = "PCX2D palette has invalid dimensions: " + input.string();
        return false;
    }

    const std::size_t width = static_cast<std::size_t>(xMax - xMin) + 1;
    const std::size_t height = static_cast<std::size_t>(yMax - yMin) + 1;

    if (width != Palette2D::Width) {
        error = "PCX2D palette '" + definition.id + "' must be 256 pixels wide";
        return false;
    }

    if (height == 0 || image.pixels.size() != width * height) {
        error = "PCX2D palette '" + definition.id + "' has inconsistent image data";
        return false;
    }

    Palette2D decodedPalette(height);

    for (std::size_t x = 0; x + 1 < width; ++x) {
        for (std::size_t y = 0; y + 1 < height; ++y) {
            const std::size_t modifiedY = y > 0 ? y - 1 : 0;

            const std::uint8_t colorIndex = image.pixels[modifiedY * width + x];

            if (colorIndex != 255) {
                decodedPalette.at(x, y) = image.palette[colorIndex];
            } else {
                decodedPalette.at(x, y) = PaletteColor{};
            }
        }
    }

    palette = std::move(decodedPalette);

    return true;
}
