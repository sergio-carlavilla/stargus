// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PcxToPngConverter.h"

#include <png.h>

#include <array>
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
            (
                static_cast<std::uint16_t>(data[offset + 1])
                << 8
            )
        );
    }

}

bool PcxToPngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open PCX input: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    );

    if (data.size() < PcxHeaderSize + PcxPaletteBlockSize) {
        error = "PCX file is too small";
        return false;
    }

    const unsigned char manufacturer = data[0];
    const unsigned char encoding = data[2];
    const unsigned char bitsPerPixel = data[3];

    const std::uint16_t xMin = readLittleEndian16(data, 4);
    const std::uint16_t yMin = readLittleEndian16(data, 6);
    const std::uint16_t xMax = readLittleEndian16(data, 8);
    const std::uint16_t yMax = readLittleEndian16(data, 10);
    const std::uint16_t bytesPerLine = readLittleEndian16(data, 66);
    const unsigned char planes = data[65];

    if (manufacturer != 0x0a) {
        error = "Unsupported PCX manufacturer";
        return false;
    }

    if (encoding != 1) {
        error = "Unsupported PCX encoding";
        return false;
    }

    if (bitsPerPixel != 8) {
        error = "Only 8-bit PCX images are supported";
        return false;
    }

    if (planes != 1) {
        error = "Only single-plane PCX images are supported";
        return false;
    }

    if (xMax < xMin || yMax < yMin) {
        error = "Invalid PCX dimensions";
        return false;
    }

    const std::size_t width = static_cast<std::size_t>(xMax - xMin) + 1;
    const std::size_t height = static_cast<std::size_t>(yMax - yMin) + 1;

    if (width == 0 || height == 0) {
        error = "Invalid PCX dimensions";
        return false;
    }

    if (bytesPerLine < width) {
        error = "Invalid PCX bytes-per-line value";
        return false;
    }

    if (height > std::numeric_limits<std::size_t>::max() / bytesPerLine) {
        error = "PCX image dimensions are too large";
        return false;
    }

    const std::size_t decodedSize = height * bytesPerLine;
    const std::size_t paletteOffset = data.size() - PcxPaletteBlockSize;

    if (data[paletteOffset] != 0x0c) {
        error = "PCX 256-color palette was not found";
        return false;
    }

    std::vector<unsigned char> decoded;
    decoded.reserve(decodedSize);

    std::size_t position = PcxHeaderSize;

    while (decoded.size() < decodedSize && position < paletteOffset) {
        unsigned char value = data[position++];
        std::size_t count = 1;

        if ((value & 0xc0) == 0xc0) {
            count = value & 0x3f;

            if (count == 0) {
                error = "Invalid PCX RLE sequence";
                return false;
            }

            if (position >= paletteOffset) {
                error = "Truncated PCX RLE sequence";
                return false;
            }

            value = data[position++];
        }

        if (count > decodedSize - decoded.size()) {
            error = "PCX RLE data exceeds expected image size";
            return false;
        }

        decoded.insert(
            decoded.end(),
            count,
            value
        );
    }

    if (decoded.size() != decodedSize) {
        error = "PCX image data is truncated";
        return false;
    }

    std::vector<unsigned char> pixels(
        width * height
    );

    for (std::size_t y = 0; y < height; ++y) {
        const std::size_t sourceOffset = y * bytesPerLine;
        const std::size_t destinationOffset = y * width;

        for (std::size_t x = 0; x < width; ++x) {
            pixels[destinationOffset + x] = decoded[sourceOffset + x];
        }
    }

    std::array<unsigned char, 256 * 4> palette{};

    const std::size_t paletteDataOffset = paletteOffset + 1;

    for (std::size_t index = 0; index < 256; ++index) {
        const std::size_t source = paletteDataOffset + index * 3;
        const std::size_t destination = index * 4;

        palette[destination + 0] = data[source + 0];
        palette[destination + 1] = data[source + 1];
        palette[destination + 2] = data[source + 2];
        palette[destination + 3] = index == 0 ? 0 : 255;
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = static_cast<png_uint_32>(width);
    image.height = static_cast<png_uint_32>(height);
    image.format = PNG_FORMAT_RGBA_COLORMAP;
    image.colormap_entries = 256;

    if (!png_image_write_to_file(
            &image,
            output.string().c_str(),
            0,
            pixels.data(),
            static_cast<png_int_32>(width),
            palette.data()
        )) {
        error = "Could not write PNG output: " + std::string(image.message);

        png_image_free(&image);

        return false;
    }

    png_image_free(&image);

    return true;
}
