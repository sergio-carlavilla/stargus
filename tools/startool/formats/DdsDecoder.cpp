// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "DdsDecoder.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

    constexpr std::size_t DdsHeaderSize = 128;

    constexpr std::uint32_t DdsPixelFormatAlphaPixels = 0x1;
    constexpr std::uint32_t DdsPixelFormatFourCc = 0x4;
    constexpr std::uint32_t DdsPixelFormatRgb = 0x40;

    constexpr std::uint32_t DdsCaps2CubeMap = 0x200;
    constexpr std::uint32_t DdsCaps2Volume = 0x200000;

    std::uint32_t readLittleEndian32(const std::vector<std::uint8_t> &data, std::size_t offset)
    {
        return static_cast<std::uint32_t>(data[offset]) |
            (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
            (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
            (static_cast<std::uint32_t>(data[offset + 3]) << 24);
    }

    std::uint32_t fourCc(const char *code)
    {
        return static_cast<std::uint32_t>(static_cast<unsigned char>(code[0])) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(code[1])) << 8) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(code[2])) << 16) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(code[3])) << 24);
    }

    struct Rgba
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
        std::uint8_t alpha = 0;
    };

    Rgba expand565(std::uint16_t color)
    {
        const unsigned red = (color >> 11) & 0x1f;
        const unsigned green = (color >> 5) & 0x3f;
        const unsigned blue = color & 0x1f;

        return Rgba{
            static_cast<std::uint8_t>((red << 3) | (red >> 2)),
            static_cast<std::uint8_t>((green << 2) | (green >> 4)),
            static_cast<std::uint8_t>((blue << 3) | (blue >> 2)),
            255
        };
    }

    std::uint8_t mix(unsigned first, unsigned second, unsigned firstWeight, unsigned secondWeight)
    {
        const unsigned total = firstWeight + secondWeight;
        return static_cast<std::uint8_t>((first * firstWeight + second * secondWeight + total / 2) / total);
    }

    Rgba mix(const Rgba &first, const Rgba &second, unsigned firstWeight, unsigned secondWeight)
    {
        return Rgba{
            mix(first.red, second.red, firstWeight, secondWeight),
            mix(first.green, second.green, firstWeight, secondWeight),
            mix(first.blue, second.blue, firstWeight, secondWeight),
            255
        };
    }

    // DXT1 colour block. DXT3/DXT5 always use the four-colour mode
    std::array<Rgba, 16> decodeColorBlock(const std::uint8_t *block, bool allowPunchThrough)
    {
        const std::uint16_t first = static_cast<std::uint16_t>(block[0] | (block[1] << 8));
        const std::uint16_t second = static_cast<std::uint16_t>(block[2] | (block[3] << 8));

        std::array<Rgba, 4> colors{};
        colors[0] = expand565(first);
        colors[1] = expand565(second);

        if (first > second || !allowPunchThrough) {
            colors[2] = mix(colors[0], colors[1], 2, 1);
            colors[3] = mix(colors[0], colors[1], 1, 2);
        } else {
            colors[2] = mix(colors[0], colors[1], 1, 1);
            colors[3] = Rgba{0, 0, 0, 0};
        }

        std::array<Rgba, 16> pixels{};
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                pixels[row * 4 + column] = colors[(block[4 + row] >> (column * 2)) & 0x3];
            }
        }

        return pixels;
    }

    std::array<std::uint8_t, 16> decodeExplicitAlphaBlock(const std::uint8_t *block)
    {
        std::array<std::uint8_t, 16> alpha{};
        for (std::size_t index = 0; index < 16; ++index) {
            const unsigned nibble = (block[index / 2] >> ((index % 2) * 4)) & 0xf;
            alpha[index] = static_cast<std::uint8_t>(nibble * 17);
        }
        return alpha;
    }

    std::array<std::uint8_t, 16> decodeInterpolatedAlphaBlock(const std::uint8_t *block)
    {
        std::array<std::uint8_t, 8> levels{};
        levels[0] = block[0];
        levels[1] = block[1];

        if (levels[0] > levels[1]) {
            for (unsigned index = 2; index < 8; ++index) {
                levels[index] = mix(levels[0], levels[1], 8 - index, index - 1);
            }
        } else {
            for (unsigned index = 2; index < 6; ++index) {
                levels[index] = mix(levels[0], levels[1], 6 - index, index - 1);
            }
            levels[6] = 0;
            levels[7] = 255;
        }

        std::uint64_t indices = 0;
        for (std::size_t byte = 0; byte < 6; ++byte) {
            indices |= static_cast<std::uint64_t>(block[2 + byte]) << (8 * byte);
        }

        std::array<std::uint8_t, 16> alpha{};
        for (std::size_t index = 0; index < 16; ++index) {
            alpha[index] = levels[(indices >> (3 * index)) & 0x7];
        }
        return alpha;
    }

    unsigned maskShift(std::uint32_t mask)
    {
        unsigned shift = 0;
        while (mask != 0 && (mask & 1) == 0) {
            mask >>= 1;
            ++shift;
        }
        return shift;
    }

    std::uint8_t maskChannel(std::uint32_t value, std::uint32_t mask)
    {
        if (mask == 0) {
            return 0;
        }
        const std::uint32_t maximum = mask >> maskShift(mask);
        const std::uint32_t channel = (value & mask) >> maskShift(mask);
        return static_cast<std::uint8_t>((static_cast<std::uint64_t>(channel) * 255 + maximum / 2) / maximum);
    }

}

bool DdsDecoder::decode(
    const std::filesystem::path &input,
    DdsImage &image,
    std::string &error
) const
{
    std::ifstream stream(input, std::ios::binary);
    if (!stream) {
        error = "Could not open DDS input: " + input.string();
        return false;
    }

    const std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>()
    );

    if (data.size() < DdsHeaderSize || readLittleEndian32(data, 0) != fourCc("DDS ")) {
        error = "Not a DDS file: " + input.string();
        return false;
    }

    if (readLittleEndian32(data, 4) != 124 || readLittleEndian32(data, 76) != 32) {
        error = "DDS file has an invalid header size: " + input.string();
        return false;
    }

    const std::uint32_t height = readLittleEndian32(data, 12);
    const std::uint32_t width = readLittleEndian32(data, 16);
    const std::uint32_t pixelFormatFlags = readLittleEndian32(data, 80);
    const std::uint32_t pixelFourCc = readLittleEndian32(data, 84);
    const std::uint32_t caps2 = readLittleEndian32(data, 112);

    if (width == 0 || height == 0 || width > 16384 || height > 16384) {
        error = "DDS file has invalid dimensions: " + input.string();
        return false;
    }

    if ((caps2 & (DdsCaps2CubeMap | DdsCaps2Volume)) != 0) {
        error = "DDS cube maps and volume textures are not supported: " + input.string();
        return false;
    }

    image.width = width;
    image.height = height;
    image.rgba.assign(static_cast<std::size_t>(width) * height * 4, 0);

    const auto store = [&image](std::size_t x, std::size_t y, const Rgba &color) {
        const std::size_t offset = (y * image.width + x) * 4;
        image.rgba[offset] = color.red;
        image.rgba[offset + 1] = color.green;
        image.rgba[offset + 2] = color.blue;
        image.rgba[offset + 3] = color.alpha;
    };

    if ((pixelFormatFlags & DdsPixelFormatFourCc) != 0) {
        std::size_t blockSize = 0;

        if (pixelFourCc == fourCc("DXT1")) {
            blockSize = 8;
        } else if (pixelFourCc == fourCc("DXT3") || pixelFourCc == fourCc("DXT5")) {
            blockSize = 16;
        } else {
            const char code[] = {
                static_cast<char>(pixelFourCc & 0xff),
                static_cast<char>((pixelFourCc >> 8) & 0xff),
                static_cast<char>((pixelFourCc >> 16) & 0xff),
                static_cast<char>((pixelFourCc >> 24) & 0xff),
                '\0'
            };
            error = "Unsupported DDS compression '" + std::string(code) + "': " + input.string();
            return false;
        }

        const std::size_t blocksWide = (width + 3) / 4;
        const std::size_t blocksHigh = (height + 3) / 4;

        if (data.size() - DdsHeaderSize < blocksWide * blocksHigh * blockSize) {
            error = "DDS file is truncated: " + input.string();
            return false;
        }

        for (std::size_t blockY = 0; blockY < blocksHigh; ++blockY) {
            for (std::size_t blockX = 0; blockX < blocksWide; ++blockX) {
                const std::uint8_t *block = data.data() + DdsHeaderSize + (blockY * blocksWide + blockX) * blockSize;

                std::array<Rgba, 16> pixels{};

                if (pixelFourCc == fourCc("DXT1")) {
                    pixels = decodeColorBlock(block, true);
                } else {
                    const std::array<std::uint8_t, 16> alpha = pixelFourCc == fourCc("DXT3")
                        ? decodeExplicitAlphaBlock(block)
                        : decodeInterpolatedAlphaBlock(block);
                    pixels = decodeColorBlock(block + 8, false);
                    for (std::size_t index = 0; index < 16; ++index) {
                        pixels[index].alpha = alpha[index];
                    }
                }

                for (std::size_t row = 0; row < 4; ++row) {
                    for (std::size_t column = 0; column < 4; ++column) {
                        const std::size_t x = blockX * 4 + column;
                        const std::size_t y = blockY * 4 + row;
                        if (x < width && y < height) {
                            store(x, y, pixels[row * 4 + column]);
                        }
                    }
                }
            }
        }

        return true;
    }

    if ((pixelFormatFlags & DdsPixelFormatRgb) == 0) {
        error = "Unsupported DDS pixel format (neither FourCC nor RGB): " + input.string();
        return false;
    }

    const std::uint32_t bitsPerPixel = readLittleEndian32(data, 88);
    if (bitsPerPixel != 24 && bitsPerPixel != 32) {
        error = "Unsupported DDS bit depth " + std::to_string(bitsPerPixel) + ": " + input.string();
        return false;
    }

    const std::uint32_t redMask = readLittleEndian32(data, 92);
    const std::uint32_t greenMask = readLittleEndian32(data, 96);
    const std::uint32_t blueMask = readLittleEndian32(data, 100);
    const std::uint32_t alphaMask = (pixelFormatFlags & DdsPixelFormatAlphaPixels) != 0 ? readLittleEndian32(data, 104) : 0;

    const std::size_t bytesPerPixel = bitsPerPixel / 8;
    const std::size_t pitch = static_cast<std::size_t>(width) * bytesPerPixel;

    if (data.size() - DdsHeaderSize < pitch * height) {
        error = "DDS file is truncated: " + input.string();
        return false;
    }

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const std::size_t offset = DdsHeaderSize + y * pitch + x * bytesPerPixel;
            std::uint32_t value = 0;
            for (std::size_t byte = 0; byte < bytesPerPixel; ++byte) {
                value |= static_cast<std::uint32_t>(data[offset + byte]) << (8 * byte);
            }

            store(x, y, Rgba{
                maskChannel(value, redMask),
                maskChannel(value, greenMask),
                maskChannel(value, blueMask),
                alphaMask != 0 ? maskChannel(value, alphaMask) : static_cast<std::uint8_t>(255)
            });
        }
    }

    return true;
}
