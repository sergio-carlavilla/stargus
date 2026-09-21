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

namespace
{

    struct RgbaColor
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
        std::uint8_t alpha = 0;
    };

    RgbaColor blendAgainstBlack(const PaletteColor &color)
    {
        const std::uint8_t biggestColor = std::max(
            std::max(color.red, color.green),
            color.blue
        );

        if (biggestColor == 0 || color.blue == 0) {
            return RgbaColor{};
        }

        const double colorFactor = static_cast<double>(biggestColor) / 255.0;

        const std::uint8_t brightBlue = static_cast<std::uint8_t>(static_cast<double>(color.blue) / colorFactor);

        if (brightBlue == 0) {
            return RgbaColor{};
        }

        /*
         * Use the behaviour from the legacy code
         *
         * Color::blendAgainstReference() contains an unconditional
         * blue-channel alpha calculation after the red/green branches,
         * so the final alpha is always derived from blue
         */
        const double alpha = static_cast<double>(color.blue) / static_cast<double>(brightBlue);

        RgbaColor result;
        result.red = static_cast<std::uint8_t>(
            alpha *
            static_cast<double>(color.red)
        );

        result.green = static_cast<std::uint8_t>(
            alpha *
            static_cast<double>(color.green)
        );

        result.blue = static_cast<std::uint8_t>(
            alpha *
            static_cast<double>(color.blue)
        );

        result.alpha = static_cast<std::uint8_t>(
            alpha *
            255.0
        );

        return result;
    }

}

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

        if (!png_image_write_to_file(&image, output.string().c_str(), 0, rgbaPixels.data(), 0, nullptr)) {

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
    }

    png_image_free(&image);

    return true;
}

bool GrpToPngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    const Palette2D &palette,
    bool rgba,
    std::string &error
) const
{
    if (!rgba) {
        error = "GRP conversion with a PCX2D palette requires RGBA output";
        return false;
    }

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
        outputWidth > std::numeric_limits<std::uint32_t>::max() ||
        outputHeight > std::numeric_limits<std::uint32_t>::max()
    ) {
        error = "GRP output dimensions are too large";
        return false;
    }

    if (outputHeight > std::numeric_limits<std::size_t>::max() / outputWidth) {
        error = "GRP output image is too large";
        return false;
    }

    std::vector<std::uint8_t> pixels(outputWidth * outputHeight, 0);

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

    if (pixels.size() > std::numeric_limits<std::size_t>::max() / 4) {
        error = "GRP RGBA output image is too large";
        return false;
    }

    std::vector<std::uint8_t> rgbaPixels(
        pixels.size() * 4
    );

    for (std::size_t index = 0; index < pixels.size(); ++index) {
        const std::uint8_t paletteIndex = pixels[index];

        if (paletteIndex == 0) {
            continue;
        }

        const std::size_t paletteRow = static_cast<std::size_t>(paletteIndex) - 1;
        if (paletteRow >= palette.height()) {
            error = "GRP palette index exceeds PCX2D palette height";
            return false;
        }

        const PaletteColor &color = palette.at(0, paletteRow );
        const RgbaColor blendedColor = blendAgainstBlack(color);

        rgbaPixels[index * 4] = blendedColor.red;
        rgbaPixels[index * 4 + 1] = blendedColor.green;
        rgbaPixels[index * 4 + 2] = blendedColor.blue;
        rgbaPixels[index * 4 + 3] = blendedColor.alpha;
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = static_cast<png_uint_32>(outputWidth);
    image.height = static_cast<png_uint_32>(outputHeight);
    image.format = PNG_FORMAT_RGBA;

    if (!png_image_write_to_file( &image, output.string().c_str(), 0, rgbaPixels.data(), 0, nullptr)) {

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
