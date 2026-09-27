// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PcxToPngConverter.h"

#include "formats/PcxDecoder.h"

#include <png.h>
#include <zlib.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace
{

    constexpr std::size_t PcxHeaderSize = 128;

    std::uint16_t readLittleEndian16(
        const std::array<std::uint8_t, PcxHeaderSize> &header,
        std::size_t offset
    )
    {
        return static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(header[offset]) |
            (
                static_cast<std::uint16_t>(header[offset + 1]) <<
                8
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
    PcxDecoder decoder;
    PcxImage image;

    if (!decoder.decode(input, image, error)) {
        return false;
    }

    std::ifstream inputStream(
        input,
        std::ios::binary
    );

    if (!inputStream) {
        error = "Could not open PCX input: " + input.string();
        return false;
    }

    std::array<std::uint8_t, PcxHeaderSize> header{};

    inputStream.read(
        reinterpret_cast<char *>(header.data()),
        static_cast<std::streamsize>(header.size())
    );

    if (inputStream.gcount() != static_cast<std::streamsize>(header.size())) {
        error = "PCX input has an incomplete header: " + input.string();
        return false;
    }

    const std::uint16_t xMin = readLittleEndian16(header, 4);
    const std::uint16_t yMin = readLittleEndian16(header, 6);
    const std::uint16_t xMax = readLittleEndian16(header, 8);
    const std::uint16_t yMax = readLittleEndian16(header, 10);

    if (xMax < xMin || yMax < yMin) {
        error = "PCX input has invalid dimensions: " + input.string();
        return false;
    }

    const std::size_t width = static_cast<std::size_t>(xMax - xMin) + 1;
    const std::size_t height = static_cast<std::size_t>(yMax - yMin) + 1;

    if (width == 0 || height == 0) {
        error = "PCX input has empty dimensions: " + input.string();
        return false;
    }

    if (height > std::numeric_limits<std::size_t>::max() / width) {
        error = "PCX image is too large: " + input.string();
        return false;
    }

    if (image.pixels.size() != width * height) {
        error = "PCX decoded pixel count does not match dimensions: " + input.string();
        return false;
    }

    const std::filesystem::path parent = output.parent_path();

    std::error_code filesystemError;

    if (!parent.empty()) {
        std::filesystem::create_directories(parent, filesystemError);

        if (filesystemError) {
            error = "Could not create PNG output directory: " + filesystemError.message();
            return false;
        }
    }

    FILE *file = std::fopen(output.string().c_str(), "wb");
    if (file == nullptr) {
        error = "Could not open PNG output: " + output.string();
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    if (png == nullptr) {
        std::fclose(file);
        error = "Could not create PNG write structure: " + output.string();
        return false;
    }

    png_infop info =
        png_create_info_struct(
            png
        );

    if (info == nullptr) {
        png_destroy_write_struct(&png, nullptr);

        std::fclose(file);

        error = "Could not create PNG info structure: " + output.string();
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);

        std::fclose(file);

        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);

        error = "Could not write PNG output: " + output.string();
        return false;
    }

    png_init_io(png, file);

    // Match Startool 3 PngExporter::saveRGB().
    png_set_compression_level(png, Z_BEST_COMPRESSION);

    png_set_IHDR(
        png,
        info,
        static_cast<png_uint_32>(width),
        static_cast<png_uint_32>(height),
        8,
        PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    std::array<png_color, 256> palette{};
    for (std::size_t index = 0; index < palette.size(); ++index) {
        palette[index].red = image.palette[index].red;

        palette[index].green = image.palette[index].green;

        palette[index].blue = image.palette[index].blue;
    }

    // Preserve the same libpng sequence used by Startool 3.
    png_set_invalid(png, info, PNG_INFO_PLTE);

    png_set_PLTE(png, info, palette.data(), static_cast<int>(palette.size()));

    std::array<png_byte, 256> transparency{};

    transparency.fill(0xFF);

    transparency[0] = 0x00;

    // Important: Startool 3 writes all 256 transparency entries.
    png_set_tRNS(png, info, transparency.data(), static_cast<int>(transparency.size()), nullptr);

    // Do not write sRGB/gAMA/cHRM chunks. Startool 3 does not set them.
    png_write_info(png, info);

    std::vector<png_bytep> rows(height);

    for (std::size_t y = 0; y < height; ++y) {
        rows[y] = reinterpret_cast<png_bytep>(image.pixels.data() + y * width);
    }

    png_write_image(png, rows.data());
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);

    std::fclose(file);

    return true;
}
