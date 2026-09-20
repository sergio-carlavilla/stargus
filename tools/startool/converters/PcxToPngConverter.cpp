// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PcxToPngConverter.h"

#include "formats/PcxDecoder.h"

#include <png.h>

#include <array>
#include <cstddef>

bool PcxToPngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    std::string &error
) const
{
    PcxDecoder decoder;
    PcxImage pcxImage;

    if (!decoder.decode(input, pcxImage, error)) {
        return false;
    }

    std::array<unsigned char, 256 * 4> palette{};
    for (std::size_t index = 0; index < pcxImage.palette.size(); ++index) {
        const PaletteColor &color = pcxImage.palette[index];

        palette[index * 4] = color.red;
        palette[index * 4 + 1] = color.green;
        palette[index * 4 + 2] = color.blue;
        palette[index * 4 + 3] = index == 0 ? 0 : 255;
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = pcxImage.width;
    image.height = pcxImage.height;
    image.format = PNG_FORMAT_RGBA_COLORMAP;
    image.colormap_entries = 256;

    if (!png_image_write_to_file(
            &image,
            output.string().c_str(),
            0,
            pcxImage.pixels.data(),
            pcxImage.width,
            palette.data()
        )) {

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
