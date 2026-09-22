// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "GrpFrameToPngConverter.h"

#include "formats/GrpDecoder.h"

#include <png.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <vector>

bool GrpFrameToPngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    const Palette &palette,
    std::size_t frameIndex,
    bool rgba,
    std::string &error
) const
{
    GrpDecoder decoder;
    GrpImage grpImage;

    if (!decoder.decode(input, grpImage, error)) {
        return false;
    }

    /*
     * Preserve the legacy Widgets::convert() behavior
     *
     * Startool legacy loads dialog GRPs with removeDuplicates=true
     * Duplicate frames are identified by their GRP data offset and
     * removed before the frame numbers from dlgs_race.json are applied
     */
    std::set<std::uint32_t> dataOffsets;
    std::vector<const GrpFrame *> uniqueFrames;
    uniqueFrames.reserve(grpImage.frames.size());

    for (const GrpFrame &candidate : grpImage.frames) {
        if (dataOffsets.insert(candidate.dataOffset).second) {
            uniqueFrames.push_back(&candidate);
        }
    }

    if (frameIndex >= uniqueFrames.size()) {
        error = "GRP frame index " + std::to_string(frameIndex) + " is out of range after duplicate removal";
        return false;
    }

    const GrpFrame &frame = *uniqueFrames[frameIndex];

    const std::size_t width = frame.width;
    const std::size_t height = frame.height;

    if (width == 0 || height == 0) {
        error = "GRP frame has invalid dimensions";
        return false;
    }

    if (
        width > std::numeric_limits<std::uint32_t>::max() ||
        height > std::numeric_limits<std::uint32_t>::max()
    ) {
        error = "GRP frame dimensions are too large";
        return false;
    }

    if (
        height >
        std::numeric_limits<std::size_t>::max() /
        width
    ) {
        error = "GRP frame image is too large";
        return false;
    }

    const std::size_t pixelCount = width * height;

    if (frame.pixels.size() != pixelCount) {
        error = "GRP frame contains inconsistent pixel data";
        return false;
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = static_cast<png_uint_32>(width);
    image.height = static_cast<png_uint_32>(height);

    if (rgba) {
        if (pixelCount > std::numeric_limits<std::size_t>::max() / 4) {
            error = "GRP frame RGBA output image is too large";
            return false;
        }

        std::vector<std::uint8_t> rgbaPixels(pixelCount * 4);

        for (std::size_t index = 0; index < pixelCount; ++index) {
            const std::uint8_t paletteIndex = frame.pixels[index];

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

        if (!png_image_write_to_file(&image, output.string().c_str(), 0, frame.pixels.data(), width, colorMap.data())) {
            error = "Could not write PNG file: ";
            error += image.message[0] != '\0' ? image.message : output.string();

            png_image_free(&image);
            return false;
        }
    }

    png_image_free(&image);

    return true;
}
