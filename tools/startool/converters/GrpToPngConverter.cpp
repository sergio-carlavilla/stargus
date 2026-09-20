// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "GrpToPngConverter.h"

#include "formats/GrpDecoder.h"

#include <png.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

bool GrpToPngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    const Palette &palette,
    bool rgba,
    std::string &error
) const
{
    GrpDecoder decoder;
    GrpImage grpImage;

    if (!decoder.decode(input, grpImage, error)) {
        return false;
    }

    if (grpImage.frames.empty()) {
        error = "GRP image contains no frames";
        return false;
    }

    std::size_t frameWidth = grpImage.maximumWidth;
    std::size_t frameHeight = grpImage.maximumHeight;

    if (grpImage.uncompressed) {
        frameWidth = 0;
        frameHeight = 0;

        for (const GrpFrame &frame : grpImage.frames) {
            frameWidth = std::max(
                frameWidth,
                static_cast<std::size_t>(frame.xOffset) +
                static_cast<std::size_t>(frame.width)
            );

            frameHeight = std::max(
                frameHeight,
                static_cast<std::size_t>(frame.yOffset) +
                static_cast<std::size_t>(frame.height)
            );
        }
    }

    if (frameWidth == 0 || frameHeight == 0) {
        error = "GRP image has invalid frame dimensions";
        return false;
    }

    const std::size_t frameCount = grpImage.frames.size();
    const std::size_t imagesPerRow = frameCount < 17 ? 1 : 17;
    const std::size_t imageRows = (frameCount + imagesPerRow - 1) / imagesPerRow;

    if (
        frameWidth >
        std::numeric_limits<std::size_t>::max() /
        imagesPerRow
    ) {
        error = "GRP output width is too large";
        return false;
    }

    if (
        frameHeight >
        std::numeric_limits<std::size_t>::max() /
        imageRows
    ) {
        error = "GRP output height is too large";
        return false;
    }

    const std::size_t outputWidth = frameWidth * imagesPerRow;
    const std::size_t outputHeight = frameHeight * imageRows;

    if (
        outputWidth >
        std::numeric_limits<std::uint32_t>::max() ||
        outputHeight >
        std::numeric_limits<std::uint32_t>::max()
    ) {
        error = "GRP output dimensions are too large";
        return false;
    }

    if (
        outputHeight >
        std::numeric_limits<std::size_t>::max() /
        outputWidth
    ) {
        error = "GRP output image is too large";
        return false;
    }

    std::vector<std::uint8_t> pixels(
        outputWidth * outputHeight,
        0
    );

    for (std::size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        const GrpFrame &frame = grpImage.frames[frameIndex];

        const std::size_t destinationColumn = frameIndex % imagesPerRow;
        const std::size_t destinationRow = frameIndex / imagesPerRow;

        for (std::size_t y = 0; y < frame.height; ++y) {
            for (std::size_t x = 0; x < frame.width; ++x) {

                const std::size_t sourcePosition = y * static_cast<std::size_t>(frame.width) + x;

                const std::size_t destinationX = destinationColumn * frameWidth + static_cast<std::size_t>(frame.xOffset) + x;

                const std::size_t destinationY = destinationRow * frameHeight + static_cast<std::size_t>(frame.yOffset) + y;

                const std::size_t destinationPosition = destinationY * outputWidth + destinationX;

                pixels[destinationPosition] = frame.pixels[sourcePosition];
            }
        }
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = static_cast<png_uint_32>(outputWidth);
    image.height = static_cast<png_uint_32>(outputHeight);

    if (rgba) {
        if (pixels.size() > std::numeric_limits<std::size_t>::max() / 4) {
            error = "GRP RGBA output image is too large";
            return false;
        }

        std::vector<std::uint8_t> rgbaPixels(
            pixels.size() * 4
        );

        for (std::size_t index = 0; index < pixels.size(); ++index) {
            const std::uint8_t paletteIndex = pixels[index];

            const PaletteColor &color = palette[paletteIndex];
            rgbaPixels[index * 4] = color.red;
            rgbaPixels[index * 4 + 1] = color.green;
            rgbaPixels[index * 4 + 2] = color.blue;
            rgbaPixels[index * 4 + 3] = paletteIndex == 0 ? 0 : 255;
        }

        image.format = PNG_FORMAT_RGBA;

        if (!png_image_write_to_file(
                &image,
                output.string().c_str(),
                0,
                rgbaPixels.data(),
                0,
                nullptr
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
    } else {
        std::array<std::uint8_t, 256 * 4> colorMap{};

        for (std::size_t index = 0; index < palette.size(); ++index) {
            const PaletteColor &color = palette[index];
            colorMap[index * 4] = color.red;
            colorMap[index * 4 + 1] = color.green;
            colorMap[index * 4 + 2] = color.blue;
            colorMap[index * 4 + 3] = index == 0 ? 0 : 255;
        }

        image.format = PNG_FORMAT_RGBA_COLORMAP;
        image.colormap_entries = 256;

        if (!png_image_write_to_file(
                &image,
                output.string().c_str(),
                0,
                pixels.data(),
                outputWidth,
                colorMap.data()
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
    }

    png_image_free(&image);

    return true;
}
