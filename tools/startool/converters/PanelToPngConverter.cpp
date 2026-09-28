// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PanelToPngConverter.h"

#include <png.h>
#include <zlib.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace
{
    unsigned char *createPanel(int width, int height)
    {
        const std::size_t size =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height) *
            4U;

        auto *buffer = static_cast<unsigned char *>(std::malloc(size));

        if (buffer == nullptr) {
            return nullptr;
        }

        std::memset(buffer, 0, size);

        auto setPixel = [buffer, width](
            int x,
            int y,
            unsigned char red,
            unsigned char green,
            unsigned char blue,
            unsigned char alpha
        ) {
            const std::size_t offset =
                static_cast<std::size_t>(y) *
                static_cast<std::size_t>(width) *
                4U +
                static_cast<std::size_t>(x) *
                4U;

            buffer[offset + 0] = red;
            buffer[offset + 1] = green;
            buffer[offset + 2] = blue;
            buffer[offset + 3] = alpha;
        };

        auto setBorderPixel = [&setPixel](int x, int y) {
            setPixel(x, y, 0x00, 0x08, 0x40, 0xff);
        };

        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                setPixel(x, y, 0x00, 0x08, 0x40, 0x80);
            }
        }

        for (int x = 3; x < width - 3; ++x) {
            setBorderPixel(x, 0);
            setBorderPixel(x, height - 1);
        }

        for (int y = 3; y < height - 3; ++y) {
            setBorderPixel(0, y);
            setBorderPixel(width - 1, y);
        }

        // Top left.
        setBorderPixel(1, 1);
        setBorderPixel(2, 1);
        setBorderPixel(1, 2);

        // Top right.
        setBorderPixel(width - 3, 1);
        setBorderPixel(width - 2, 1);
        setBorderPixel(width - 2, 2);

        // Bottom left.
        setBorderPixel(1, height - 3);
        setBorderPixel(1, height - 2);
        setBorderPixel(2, height - 2);

        // Bottom right.
        setBorderPixel(width - 3, height - 2);
        setBorderPixel(width - 2, height - 2);
        setBorderPixel(width - 2, height - 3);

        return buffer;
    }
}

bool PanelToPngConverter::convert(
    const std::filesystem::path &output,
    int width,
    int height,
    std::string &error
) const
{
    if (width < 7 || height < 7) {
        error = "Panel dimensions must both be at least 7 pixels";
        return false;
    }

    FILE *file = std::fopen(output.string().c_str(), "wb");

    if (file == nullptr) {
        error = "Could not open panel PNG for writing: " + output.string();
        return false;
    }

    png_structp png = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        nullptr,
        nullptr,
        nullptr
    );

    if (png == nullptr) {
        std::fclose(file);
        error = "Could not create PNG writer for panel: " + output.string();
        return false;
    }

    png_infop info = png_create_info_struct(png);

    if (info == nullptr) {
        png_destroy_write_struct(&png, nullptr);
        std::fclose(file);
        error = "Could not create PNG info for panel: " + output.string();
        return false;
    }

    unsigned char *buffer = nullptr;
    png_bytep *rows = nullptr;

    if (setjmp(png_jmpbuf(png))) {
        std::free(rows);
        std::free(buffer);
        png_destroy_write_struct(&png, &info);
        std::fclose(file);
        error = "Could not encode panel PNG: " + output.string();
        return false;
    }

    png_init_io(png, file);
    png_set_compression_level(png, Z_BEST_COMPRESSION);

    png_set_IHDR(
        png,
        info,
        static_cast<png_uint_32>(width),
        static_cast<png_uint_32>(height),
        8,
        PNG_COLOR_TYPE_RGB_ALPHA,
        0,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    buffer = createPanel(width, height);

    if (buffer == nullptr) {
        png_destroy_write_struct(&png, &info);
        std::fclose(file);
        error = "Could not allocate panel pixel buffer";
        return false;
    }

    png_write_info(png, info);

    rows = static_cast<png_bytep *>(
        std::malloc(
            static_cast<std::size_t>(height) *
            sizeof(*rows)
        )
    );

    if (rows == nullptr) {
        std::free(buffer);
        png_destroy_write_struct(&png, &info);
        std::fclose(file);
        error = "Could not allocate panel PNG row table";
        return false;
    }

    for (int y = 0; y < height; ++y) {
        rows[y] =
            buffer +
            static_cast<std::size_t>(y) *
            static_cast<std::size_t>(width) *
            4U;
    }

    png_write_image(png, rows);
    png_write_end(png, info);

    png_destroy_write_struct(&png, &info);
    std::fclose(file);

    std::free(rows);
    std::free(buffer);

    return true;
}
