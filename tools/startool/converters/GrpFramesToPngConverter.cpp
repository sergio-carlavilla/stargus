// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "GrpFramesToPngConverter.h"

#include "formats/GrpDecoder.h"

#include <png.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <vector>

bool GrpFramesToPngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    const Palette &palette,
    const std::vector<std::size_t> &frameIndices,
    bool rgba,
    std::string &error
) const
{
    if (frameIndices.empty()) {
        error = "GRP stitched conversion requires at least one frame";
        return false;
    }

    GrpDecoder decoder;
    GrpImage grpImage;

    if (!decoder.decode(input, grpImage, error)) {
        return false;
    }

    /*
     * Preserve startoll legacy Widgets::convert() behavior
     *
     * Dialog GRPs are loaded with removeDuplicates=true before frame indices
     * from dlgs_race.json are resolved
     */
    std::set<std::uint32_t> dataOffsets;
    std::vector<const GrpFrame *> uniqueFrames;
    uniqueFrames.reserve(grpImage.frames.size());

    for (const GrpFrame &candidate : grpImage.frames) {
        if (dataOffsets.insert(candidate.dataOffset).second) {
            uniqueFrames.push_back(&candidate);
        }
    }

    std::vector<const GrpFrame *> selectedFrames;
    selectedFrames.reserve(frameIndices.size());

    std::size_t frameWidth = 0;
    std::size_t frameHeight = 0;

    for (const std::size_t frameIndex : frameIndices) {
        if (frameIndex >= uniqueFrames.size()) {
            error = "GRP frame index " + std::to_string(frameIndex) + " is out of range after duplicate removal";
            return false;
        }

        const GrpFrame *frame = uniqueFrames[frameIndex];

        const std::size_t width =
            static_cast<std::size_t>(frame->xOffset) +
            static_cast<std::size_t>(frame->width);

        const std::size_t height =
            static_cast<std::size_t>(frame->yOffset) +
            static_cast<std::size_t>(frame->height);

        if (width > frameWidth) {
            frameWidth = width;
        }

        if (height > frameHeight) {
            frameHeight = height;
        }

        selectedFrames.push_back(frame);
    }

    if (frameWidth == 0 || frameHeight == 0) {
        error = "GRP stitched image has invalid frame dimensions";
        return false;
    }

    if (
        frameWidth >
        std::numeric_limits<std::size_t>::max() /
        selectedFrames.size()
    ) {
        error = "GRP stitched output width is too large";
        return false;
    }

    const std::size_t outputWidth =
        frameWidth *
        selectedFrames.size();

    const std::size_t outputHeight = frameHeight;

    if (
        outputWidth > std::numeric_limits<std::uint32_t>::max() ||
        outputHeight > std::numeric_limits<std::uint32_t>::max()
    ) {
        error = "GRP stitched output dimensions are too large";
        return false;
    }

    if (
        outputHeight >
        std::numeric_limits<std::size_t>::max() /
        outputWidth
    ) {
        error = "GRP stitched output image is too large";
        return false;
    }

    std::vector<std::uint8_t> pixels(
        outputWidth * outputHeight,
        0
    );

    for (
        std::size_t selectedIndex = 0;
        selectedIndex < selectedFrames.size();
        ++selectedIndex
    ) {
        const GrpFrame &frame = *selectedFrames[selectedIndex];

        const std::size_t expectedPixelCount =
            static_cast<std::size_t>(frame.width) *
            static_cast<std::size_t>(frame.height);

        if (frame.pixels.size() != expectedPixelCount) {
            error = "GRP frame contains inconsistent pixel data";
            return false;
        }

        for (std::size_t y = 0; y < frame.height; ++y) {
            for (std::size_t x = 0; x < frame.width; ++x) {
                const std::size_t sourcePosition = y * static_cast<std::size_t>(frame.width) + x;
                const std::size_t destinationX = selectedIndex * frameWidth + static_cast<std::size_t>(frame.xOffset) + x;
                const std::size_t destinationY = static_cast<std::size_t>(frame.yOffset) + y;
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
            error = "GRP stitched RGBA output image is too large";
            return false;
        }

        std::vector<std::uint8_t> rgbaPixels(
            pixels.size() * 4
        );

        for (std::size_t index = 0; index < pixels.size(); ++index) {
            const std::uint8_t paletteIndex = pixels[index];

            if (paletteIndex == 0) {
                rgbaPixels[index * 4] = 0;
                rgbaPixels[index * 4 + 1] = 0;
                rgbaPixels[index * 4 + 2] = 0;
                rgbaPixels[index * 4 + 3] = 0;
                continue;
            }

            const PaletteColor &color = palette[paletteIndex];

            rgbaPixels[index * 4] = color.red;
            rgbaPixels[index * 4 + 1] = color.green;
            rgbaPixels[index * 4 + 2] = color.blue;
            rgbaPixels[index * 4 + 3] = 255;
        }

        image.format = PNG_FORMAT_RGBA;

        if (!png_image_write_to_file(&image, output.string().c_str(), 0, rgbaPixels.data(), 0, nullptr)) {
            error = "Could not write PNG file: ";
            error += image.message[0] != '\0' ? image.message : output.string();

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
            error += image.message[0] != '\0' ? image.message : output.string();

            png_image_free(&image);
            return false;
        }
    }

    png_image_free(&image);

    return true;
}
