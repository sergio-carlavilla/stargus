// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "WpePaletteLoader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>

bool WpePaletteLoader::load(
    const std::filesystem::path &input,
    const PaletteDefinition &definition,
    Palette &palette,
    std::string &error
) const
{
    constexpr std::size_t colorCount = 256;
    constexpr std::size_t bytesPerColor = 4;
    constexpr std::size_t paletteSize = colorCount * bytesPerColor;

    std::ifstream file(
        input,
        std::ios::binary
    );

    if (!file) {
        error = "Could not open WPE palette: " + input.string();
        return false;
    }

    std::array<std::uint8_t, paletteSize> data{};

    file.read(
        reinterpret_cast<char *>(data.data()),
        static_cast<std::streamsize>(data.size())
    );

    if (file.gcount() != static_cast<std::streamsize>(data.size())) {
        error = "WPE palette '" + definition.id + "' must contain exactly 1024 bytes";
        return false;
    }

    char extraByte = 0;
    if (file.read(&extraByte, 1)) {
        error = "WPE palette '" + definition.id + "' must contain exactly 1024 bytes";
        return false;
    }

    Palette decodedPalette{};

    for (std::size_t index = 0; index < colorCount; ++index) {
        const std::size_t offset = index * bytesPerColor;

        decodedPalette[index].red = data[offset];
        decodedPalette[index].green = data[offset + 1];
        decodedPalette[index].blue = data[offset + 2];
    }

    palette = decodedPalette;

    return true;
}
