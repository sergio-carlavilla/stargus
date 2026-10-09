// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "DdsToPngConverter.h"

#include "formats/DdsDecoder.h"

#include <png.h>
#include <zlib.h>

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

bool DdsToPngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    std::string &error,
    int cropX,
    int cropY,
    int cropWidth,
    int cropHeight,
    int bottomAlignHeight
) const
{
    DdsDecoder decoder;
    DdsImage image;

    if (!decoder.decode(input, image, error)) {
        return false;
    }

    // Crop decoded RGBA pixels without copying or discarding alpha
    std::size_t x = 0;
    std::size_t y = 0;
    std::size_t width = image.width;
    std::size_t height = image.height;

    if (cropX != -1 || cropY != -1 || cropWidth != -1 || cropHeight != -1) {
        if (cropX < 0 || cropY < 0 || cropWidth <= 0 || cropHeight <= 0) {
            error = "Invalid DDS crop rectangle";
            return false;
        }

        x = static_cast<std::size_t>(cropX);
        y = static_cast<std::size_t>(cropY);
        width = static_cast<std::size_t>(cropWidth);
        height = static_cast<std::size_t>(cropHeight);

        if (x >= image.width || y >= image.height ||
            width > image.width - x || height > image.height - y) {
            error = "DDS crop rectangle exceeds input dimensions";
            return false;
        }
    }

    // Bottom-align a short CASC console on the original 640x480 canvas
    // Newly introduced rows must be fully transparent, not stretched pixels
    std::size_t outputHeight = height;
    if (bottomAlignHeight != -1) {
        if (bottomAlignHeight < 1 || bottomAlignHeight > 16384 || static_cast<std::size_t>(bottomAlignHeight) < height) {
            error = "Invalid DDS bottom-align height";
            return false;
        }
        outputHeight = static_cast<std::size_t>(bottomAlignHeight);
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

    png_infop info = png_create_info_struct(png);
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
    png_set_compression_level(png, Z_BEST_COMPRESSION);

    // RGBA keeps the transparency stored in the DDS alpha channel
    png_set_IHDR(
        png,
        info,
        static_cast<png_uint_32>(width),
        static_cast<png_uint_32>(outputHeight),
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png, info);

    if (outputHeight > height) {
        const std::vector<std::uint8_t> transparentRow(width * 4, 0);
        for (std::size_t row = 0; row < outputHeight - height; ++row) {
            png_write_row(png, transparentRow.data());
        }
    }

    for (std::size_t row = 0; row < height; ++row) {
        const std::size_t offset = ((y + row) * image.width + x) * 4;
        png_write_row(png, image.rgba.data() + offset);
    }

    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);

    if (std::fclose(file) != 0) {
        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);
        error = "Could not close PNG output: " + output.string();
        return false;
    }

    return true;
}
