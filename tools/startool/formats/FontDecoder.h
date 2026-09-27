// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef FONT_DECODER_H
#define FONT_DECODER_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct FontImage
{
    std::size_t width = 0;
    std::size_t height = 0;

    std::uint8_t lowIndex = 0;
    std::uint8_t highIndex = 0;
    std::uint8_t maxWidth = 0;
    std::uint8_t maxHeight = 0;

    std::vector<std::uint8_t> pixels;
};

class FontDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            FontImage &image,
            std::string &error
        ) const;

};

#endif // FONT_DECODER_H
