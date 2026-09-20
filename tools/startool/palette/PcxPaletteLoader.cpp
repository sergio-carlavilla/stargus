// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PcxPaletteLoader.h"

#include "formats/PcxDecoder.h"

#include <cstddef>

bool PcxPaletteLoader::load(
    const std::filesystem::path &input,
    const PaletteDefinition &definition,
    Palette &palette,
    std::string &error
) const
{
    PcxDecoder decoder;
    PcxImage image;

    if (!decoder.decode(input, image, error)) {
        return false;
    }

    palette = image.palette;

    if (!definition.mapping) {
        return true;
    }

    const PaletteMapping &mapping = *definition.mapping;
    const std::size_t length = static_cast<std::size_t>(mapping.length);
    const std::size_t start = static_cast<std::size_t>(mapping.start);
    const std::size_t index = static_cast<std::size_t>(mapping.index);

    if (start + length > palette.size()) {
        error = "Palette mapping exceeds destination palette for '" + definition.id + "'";
        return false;
    }

    if (index > image.pixels.size() / length) {
        error = "Palette mapping exceeds PCX image data for '" + definition.id + "'";
        return false;
    }

    const std::size_t sourceOffset = index * length;
    if (sourceOffset + length > image.pixels.size()) {
        error = "Palette mapping exceeds PCX image data for '" + definition.id + "'";
        return false;
    }

    for (std::size_t i = 0; i < length; ++i) {
        const std::uint8_t colorIndex = image.pixels[sourceOffset + i];
        palette[start + i] = palette[colorIndex];
    }

    return true;
}
