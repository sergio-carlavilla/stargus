// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PcxDecoder.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>

namespace
{

    constexpr std::size_t PcxHeaderSize = 128;
    constexpr std::size_t PcxPaletteSize = 256 * 3;
    constexpr std::size_t PcxPaletteBlockSize = 1 + PcxPaletteSize;

    std::uint16_t readLittleEndian16(
        const std::vector<unsigned char> &data,
        std::size_t offset
    )
    {
        return static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(data[offset]) |
            static_cast<std::uint16_t>(data[offset + 1]) << 8
        );
    }

}

bool PcxDecoder::decode(
    const std::filesystem::path &input,
    PcxImage &image,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open PCX file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    if (data.size() < PcxHeaderSize + PcxPaletteBlockSize) {
        error = "PCX file is too small";
        return false;
    }

    const unsigned char manufacturer = data[0];
    const unsigned char encoding = data[2];
    const unsigned char bitsPerPixel = data[3];
    const unsigned char planes = data[65];

    if (manufacturer != 0x0a) {
        error = "Unsupported PCX manufacturer";
        return false;
    }

    if (encoding != 1) {
        error = "Unsupported PCX encoding";
        return false;
    }

    if (bitsPerPixel != 8 || planes != 1) {
        error = "Only 8-bit single-plane PCX images are supported";
        return false;
    }

    const std::uint16_t xMin = readLittleEndian16(data, 4);
    const std::uint16_t yMin = readLittleEndian16(data, 6);
    const std::uint16_t xMax = readLittleEndian16(data, 8);
    const std::uint16_t yMax = readLittleEndian16(data, 10);
    const std::uint16_t bytesPerLine = readLittleEndian16(data, 66);

    if (xMax < xMin || yMax < yMin) {
        error = "Invalid PCX image dimensions";
        return false;
    }

    const std::uint32_t width = static_cast<std::uint32_t>(xMax) - static_cast<std::uint32_t>(xMin) + 1;
    const std::uint32_t height = static_cast<std::uint32_t>(yMax) - static_cast<std::uint32_t>(yMin) + 1;

    if (width == 0 || height == 0) {
        error = "Invalid PCX image dimensions";
        return false;
    }

    if (bytesPerLine < width) {
        error = "Invalid PCX bytes-per-line value";
        return false;
    }

    if (height > std::numeric_limits<std::size_t>::max() / bytesPerLine) {
        error = "PCX image is too large";
        return false;
    }

    const std::size_t decodedSize = static_cast<std::size_t>(height) * static_cast<std::size_t>(bytesPerLine);
    const std::size_t paletteOffset = data.size() - PcxPaletteBlockSize;

    if (data[paletteOffset] != 0x0c) {
        error = "PCX palette marker was not found";
        return false;
    }

    std::vector<unsigned char> decoded;
    decoded.reserve(decodedSize);

    std::size_t position = PcxHeaderSize;

    while (decoded.size() < decodedSize) {
        if (position >= paletteOffset) {
            error = "Unexpected end of PCX image data";
            return false;
        }

        const unsigned char packet = data[position++];

        unsigned int count = 1;
        unsigned char value = packet;

        if ((packet & 0xc0) == 0xc0) {
            count = packet & 0x3f;

            if (count == 0) {
                error = "Invalid PCX RLE packet";
                return false;
            }

            if (position >= paletteOffset) {
                error = "Unexpected end of PCX RLE packet";
                return false;
            }

            value = data[position++];
        }

        if (decoded.size() + count > decodedSize) {
            error = "PCX RLE data exceeds expected image size";
            return false;
        }

        decoded.insert(decoded.end(), count, value);
    }

    image.width = static_cast<std::uint16_t>(width);
    image.height = static_cast<std::uint16_t>(height);

    image.pixels.resize(
        static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height)
    );

    for (std::size_t y = 0; y < height; ++y) {
        const std::size_t sourceOffset = y * static_cast<std::size_t>(bytesPerLine);
        const std::size_t destinationOffset = y * static_cast<std::size_t>(width);

        for (std::size_t x = 0; x < width; ++x) {
            image.pixels[destinationOffset + x] = decoded[sourceOffset + x];
        }
    }

    const std::size_t paletteDataOffset = paletteOffset + 1;
    for (std::size_t index = 0; index < image.palette.size(); ++index) {
        const std::size_t colorOffset = paletteDataOffset + index * 3;

        image.palette[index].red = data[colorOffset];
        image.palette[index].green = data[colorOffset + 1];
        image.palette[index].blue = data[colorOffset + 2];
    }

    return true;
}
