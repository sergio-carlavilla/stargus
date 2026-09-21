// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "TilesetToPngConverter.h"

#include <png.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <vector>

namespace
{

    constexpr std::size_t MiniTileWidth = 8;
    constexpr std::size_t MiniTileHeight = 8;
    constexpr std::size_t MiniTileSize = MiniTileWidth * MiniTileHeight;

    constexpr std::size_t MegaTileMiniTilesPerSide = 4;
    constexpr std::size_t MegaTileMiniTileCount = MegaTileMiniTilesPerSide * MegaTileMiniTilesPerSide;
    constexpr std::size_t MegaTileWidth = MiniTileWidth * MegaTileMiniTilesPerSide;
    constexpr std::size_t MegaTileHeight = MiniTileHeight * MegaTileMiniTilesPerSide;
    constexpr std::size_t MegaTileReferenceSize = MegaTileMiniTileCount * sizeof(std::uint16_t);

    constexpr std::size_t TilesPerRow = 16;

    bool readFile(
        const std::filesystem::path &input,
        std::vector<std::uint8_t> &data,
        std::string &error
    )
    {
        std::ifstream stream(input, std::ios::binary | std::ios::ate);

        if (!stream) {
            error = "Could not open tileset resource: " + input.string();
            return false;
        }

        const std::streamsize size = stream.tellg();
        if (size < 0) {
            error = "Could not determine tileset resource size: " + input.string();
            return false;
        }

        stream.seekg(0, std::ios::beg);

        data.resize(static_cast<std::size_t>(size));

        if (!data.empty() && !stream.read(reinterpret_cast<char *>(data.data()), size)) {
            error = "Could not read tileset resource: " + input.string();
            return false;
        }

        return true;
    }

    std::uint16_t readLittleEndian16(const std::uint8_t *data)
    {
        return static_cast<std::uint16_t>(data[0]) | static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) << 8);
    }

}

bool TilesetToPngConverter::convert(
    const std::filesystem::path &vx4Input,
    const std::filesystem::path &vr4Input,
    const std::filesystem::path &output,
    const Palette &palette,
    std::string &error
) const
{
    std::vector<std::uint8_t> vx4;
    std::vector<std::uint8_t> vr4;

    if (!readFile(vx4Input, vx4, error)) {
        return false;
    }

    if (!readFile(vr4Input, vr4, error)) {
        return false;
    }

    if (vx4.empty() || vx4.size() % MegaTileReferenceSize != 0) {
        error = "VX4 resource has invalid size: " + vx4Input.string();
        return false;
    }

    if (vr4.empty() || vr4.size() % MiniTileSize != 0) {
        error = "VR4 resource has invalid size: " + vr4Input.string();
        return false;
    }

    const std::size_t megaTileCount = vx4.size() / MegaTileReferenceSize;
    const std::size_t miniTileCount = vr4.size() / MiniTileSize;
    const std::size_t tileRows = (megaTileCount + TilesPerRow - 1) / TilesPerRow;

    if (
        TilesPerRow > std::numeric_limits<std::size_t>::max() / MegaTileWidth ||
        tileRows > std::numeric_limits<std::size_t>::max() / MegaTileHeight
    ) {
        error = "Tileset output dimensions are too large";
        return false;
    }

    const std::size_t outputWidth = TilesPerRow * MegaTileWidth;
    const std::size_t outputHeight = tileRows * MegaTileHeight;

    if (
        outputWidth != 0 &&
        outputHeight > std::numeric_limits<std::size_t>::max() / outputWidth
    ) {
        error = "Tileset output image is too large";
        return false;
    }

    std::vector<std::uint8_t> pixels(
        outputWidth * outputHeight,
        0
    );

    for (std::size_t megaTileIndex = 0; megaTileIndex < megaTileCount; ++megaTileIndex) {
        const std::size_t megaTileX = (megaTileIndex % TilesPerRow) * MegaTileWidth;
        const std::size_t megaTileY = (megaTileIndex / TilesPerRow) * MegaTileHeight;
        const std::size_t megaTileOffset = megaTileIndex * MegaTileReferenceSize;

        for (std::size_t miniTilePosition = 0; miniTilePosition < MegaTileMiniTileCount; ++miniTilePosition) {
            const std::uint16_t reference = readLittleEndian16(
                vx4.data() +
                megaTileOffset +
                miniTilePosition * sizeof(std::uint16_t)
            );

            const bool horizontallyFlipped = (reference & 0x0001U) != 0;
            const std::size_t miniTileIndex = static_cast<std::size_t>(reference >> 1);

            if (miniTileIndex >= miniTileCount) {
                error = "VX4 references VR4 minitile outside resource bounds";
                return false;
            }

            const std::size_t miniTileX = (miniTilePosition % MegaTileMiniTilesPerSide) * MiniTileWidth;
            const std::size_t miniTileY = (miniTilePosition / MegaTileMiniTilesPerSide) * MiniTileHeight;
            const std::size_t miniTileOffset = miniTileIndex * MiniTileSize;

            for (std::size_t y = 0; y < MiniTileHeight; ++y) {
                for (std::size_t x = 0; x < MiniTileWidth; ++x) {
                    const std::size_t sourceX = horizontallyFlipped ? (MiniTileWidth - 1 - x) : x;
                    const std::uint8_t paletteIndex = vr4[miniTileOffset + y * MiniTileWidth + sourceX];
                    const std::size_t destinationX = megaTileX + miniTileX + x;
                    const std::size_t destinationY = megaTileY + miniTileY + y;

                    pixels[destinationY * outputWidth + destinationX] = paletteIndex;
                }
            }
        }
    }

    std::array<std::uint8_t, 256 * 4> colorMap{};

    for (std::size_t index = 0; index < palette.size(); ++index) {
        const PaletteColor &color = palette[index];

        colorMap[index * 4] = color.red;
        colorMap[index * 4 + 1] = color.green;
        colorMap[index * 4 + 2] = color.blue;
        colorMap[index * 4 + 3] = index == 0 ? 0 : 255;
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = static_cast<png_uint_32>(outputWidth);
    image.height = static_cast<png_uint_32>(outputHeight);
    image.format = PNG_FORMAT_RGBA_COLORMAP;
    image.colormap_entries = 256;

    if (!png_image_write_to_file(&image, output.string().c_str(), 0, pixels.data(), outputWidth, colorMap.data())) {

        error = "Could not write PNG file: ";

        if (image.message[0] != '\0') {
            error += image.message;
        } else {
            error += output.string();
        }

        png_image_free(&image);
        return false;
    }

    png_image_free(&image);

    return true;
}
