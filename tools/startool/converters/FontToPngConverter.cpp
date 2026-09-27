// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "FontToPngConverter.h"

#include "formats/FontDecoder.h"

#include <png.h>
#include <zlib.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <vector>

bool FontToPngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    const Palette &palette,
    std::string &error
) const
{
    FontDecoder decoder;
    FontImage fontImage;

    if (!decoder.decode(input, fontImage, error)) {
        return false;
    }

    const std::filesystem::path parent = output.parent_path();

    std::error_code filesystemError;
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, filesystemError);
        if (filesystemError) {
            error = "Could not create font PNG output directory: " + filesystemError.message();
            return false;
        }
    }

    FILE *file = std::fopen(output.string().c_str(), "wb");
    if (file == nullptr) {
        error = "Could not open font PNG output: " + output.string();
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr) {
        std::fclose(file);
        error = "Could not create PNG write structure for font: " + output.string();
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (info == nullptr) {
        png_destroy_write_struct(&png, nullptr);
        std::fclose(file);
        error = "Could not create PNG info structure for font: " + output.string();
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        std::fclose(file);
        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);
        error = "Could not write font PNG output: " + output.string();
        return false;
    }

    png_init_io(png, file);

    // Match Startool 3 PngExporter::saveRGB().
    png_set_compression_level(png, Z_BEST_COMPRESSION);

    png_set_IHDR(
        png,
        info,
        static_cast<png_uint_32>(
            fontImage.width
        ),
        static_cast<png_uint_32>(
            fontImage.height
        ),
        8,
        PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    std::array<png_color, 256> pngPalette{};
    for (std::size_t index = 0; index < pngPalette.size(); ++index) {
        pngPalette[index].red = palette[index].red;
        pngPalette[index].green = palette[index].green;
        pngPalette[index].blue = palette[index].blue;
    }

    png_set_invalid(png, info, PNG_INFO_PLTE);
    png_set_PLTE(png, info, pngPalette.data(), static_cast<int>(pngPalette.size()));

    std::array<png_byte, 256> transparency{};

    transparency.fill(0xFF);

    // Startool legacy calls PngExporter::save(..., transparent = 255).
    transparency[255] = 0x00;

    png_set_tRNS(png, info, transparency.data(), static_cast<int>(transparency.size()), nullptr);
    png_write_info( png, info);

    std::vector<png_bytep> rows(fontImage.height);
    for (std::size_t y = 0; y < fontImage.height; ++y) {
        rows[y] = reinterpret_cast<png_bytep>(fontImage.pixels.data() + y * fontImage.width);
    }

    png_write_image(png, rows.data());
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);

    std::fclose(file);

    return true;
}
