// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "converters/DdsToPngConverter.h"
#include "formats/DdsDecoder.h"

#include <png.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    int failures = 0;

    void check(bool condition, const std::string &message)
    {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    void appendLe32(std::vector<unsigned char> &data, std::uint32_t value)
    {
        for (int shift = 0; shift < 32; shift += 8) {
            data.push_back(static_cast<unsigned char>((value >> shift) & 0xff));
        }
    }

    std::uint32_t code(const char *text)
    {
        return static_cast<std::uint32_t>(static_cast<unsigned char>(text[0])) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(text[1])) << 8) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(text[2])) << 16) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(text[3])) << 24);
    }

    std::vector<unsigned char> header(
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t pixelFlags,
        std::uint32_t fourCc,
        std::uint32_t bits = 0,
        std::array<std::uint32_t, 4> masks = {}
    )
    {
        std::vector<unsigned char> data;
        appendLe32(data, code("DDS "));
        appendLe32(data, 124);
        appendLe32(data, 0x1007);
        appendLe32(data, height);
        appendLe32(data, width);
        for (int i = 0; i < 3 + 11; ++i) {
            appendLe32(data, 0);    // pitch, depth, mipmaps, reserved
        }
        appendLe32(data, 32);
        appendLe32(data, pixelFlags);
        appendLe32(data, fourCc);
        appendLe32(data, bits);
        for (std::uint32_t mask : masks) {
            appendLe32(data, mask);
        }
        for (int i = 0; i < 5; ++i) {
            appendLe32(data, 0);    // caps, caps2, caps3, caps4, reserved2
        }
        return data;
    }

    std::filesystem::path write(const std::string &name, const std::vector<unsigned char> &data)
    {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / ("startool-dds-test-" + name);
        std::ofstream output(path, std::ios::binary);
        output.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        return path;
    }

    std::array<int, 4> pixel(const DdsImage &image, std::size_t x, std::size_t y)
    {
        const std::size_t offset = (y * image.width + x) * 4;
        return {image.rgba[offset], image.rgba[offset + 1], image.rgba[offset + 2], image.rgba[offset + 3]};
    }

    void expectPixel(const DdsImage &image, std::size_t x, std::size_t y, std::array<int, 4> expected, const std::string &what)
    {
        const std::array<int, 4> actual = pixel(image, x, y);
        check(actual == expected, what + " pixel(" + std::to_string(x) + "," + std::to_string(y) + ") = " +
            std::to_string(actual[0]) + "," + std::to_string(actual[1]) + "," + std::to_string(actual[2]) + "," + std::to_string(actual[3]));
    }

    bool decode(const std::string &name, const std::vector<unsigned char> &data, DdsImage &image, std::string &error)
    {
        const std::filesystem::path path = write(name, data);
        const bool decoded = DdsDecoder().decode(path, image, error);
        std::filesystem::remove(path);
        return decoded;
    }
}

int main()
{
    std::string error;

    {
        // DXT1, four-colour mode: red / blue end points, row 0 uses indices 0..3
        std::vector<unsigned char> data = header(4, 4, 0x4, code("DXT1"));
        data.insert(data.end(), {0x00, 0xf8, 0x1f, 0x00, 0xe4, 0x00, 0x00, 0x00});
        DdsImage image;
        check(decode("dxt1.dds", data, image, error), "DXT1 decodes: " + error);
        expectPixel(image, 0, 0, {255, 0, 0, 255}, "DXT1 colour 0");
        expectPixel(image, 1, 0, {0, 0, 255, 255}, "DXT1 colour 1");
        expectPixel(image, 2, 0, {170, 0, 85, 255}, "DXT1 colour 2");
        expectPixel(image, 3, 0, {85, 0, 170, 255}, "DXT1 colour 3");
        expectPixel(image, 3, 3, {255, 0, 0, 255}, "DXT1 last pixel");
    }

    {
        // DXT1 punch-through: colour0 <= colour1 makes index 3 transparent
        std::vector<unsigned char> data = header(4, 4, 0x4, code("DXT1"));
        data.insert(data.end(), {0x1f, 0x00, 0x00, 0xf8, 0xe4, 0x00, 0x00, 0x00});
        DdsImage image;
        check(decode("dxt1a.dds", data, image, error), "DXT1 punch-through decodes: " + error);
        expectPixel(image, 2, 0, {128, 0, 128, 255}, "DXT1 three-colour midpoint");
        expectPixel(image, 3, 0, {0, 0, 0, 0}, "DXT1 transparent");
    }

    {
        // DXT5: alpha end points 255/0, pixels 0..2 use alpha indices 0, 1, 2; white colour block
        std::vector<unsigned char> data = header(4, 4, 0x4, code("DXT5"));
        std::uint64_t indices = 0;
        const int alphaIndex[16] = {0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
        for (int i = 0; i < 16; ++i) {
            indices |= static_cast<std::uint64_t>(alphaIndex[i]) << (3 * i);
        }
        data.push_back(255);
        data.push_back(0);
        for (int byte = 0; byte < 6; ++byte) {
            data.push_back(static_cast<unsigned char>((indices >> (8 * byte)) & 0xff));
        }
        data.insert(data.end(), {0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        DdsImage image;
        check(decode("dxt5.dds", data, image, error), "DXT5 decodes: " + error);
        expectPixel(image, 0, 0, {255, 255, 255, 255}, "DXT5 opaque");
        expectPixel(image, 1, 0, {255, 255, 255, 0}, "DXT5 transparent");
        expectPixel(image, 2, 0, {255, 255, 255, 219}, "DXT5 interpolated alpha");
    }

    {
        // Uncompressed A8R8G8B8 (BGRA in memory), 2x1
        std::vector<unsigned char> data = header(2, 1, 0x41, 0, 32, {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000});
        data.insert(data.end(), {30, 20, 10, 40, 50, 100, 200, 0});
        DdsImage image;
        check(decode("bgra.dds", data, image, error), "BGRA decodes: " + error);
        expectPixel(image, 0, 0, {10, 20, 30, 40}, "BGRA first");
        expectPixel(image, 1, 0, {200, 100, 50, 0}, "BGRA second");
    }

    {
        // 5x3 DXT1 needs two blocks horizontally; pixels outside the image are clipped
        std::vector<unsigned char> data = header(5, 3, 0x4, code("DXT1"));
        data.insert(data.end(), {0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        data.insert(data.end(), {0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        DdsImage image;
        check(decode("dxt1-5x3.dds", data, image, error), "5x3 DXT1 decodes: " + error);
        check(image.width == 5 && image.height == 3 && image.rgba.size() == 5 * 3 * 4, "5x3 dimensions");
        expectPixel(image, 3, 2, {255, 0, 0, 255}, "5x3 first block");
        expectPixel(image, 4, 2, {0, 0, 255, 255}, "5x3 second block");
    }

    {
        DdsImage image;
        check(!decode("dx10.dds", header(4, 4, 0x4, code("DX10")), image, error), "DX10 is rejected");
        check(!decode("truncated.dds", header(8, 8, 0x4, code("DXT5")), image, error), "truncated data is rejected");
        check(!decode("notdds.dds", std::vector<unsigned char>(200, 0), image, error), "non-DDS input is rejected");
    }

    {
        // PNG output keeps RGBA and alpha
        std::vector<unsigned char> data = header(2, 1, 0x41, 0, 32, {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000});
        data.insert(data.end(), {30, 20, 10, 40, 50, 100, 200, 0});
        const std::filesystem::path input = write("png.dds", data);
        const std::filesystem::path output = std::filesystem::temp_directory_path() / "startool-dds-test-out.png";
        check(DdsToPngConverter().convert(input, output, error), "DDS to PNG converts: " + error);

        png_image png{};
        png.version = PNG_IMAGE_VERSION;
        std::vector<unsigned char> pixels;
        if (png_image_begin_read_from_file(&png, output.string().c_str())) {
            check((png.format & PNG_FORMAT_FLAG_ALPHA) != 0, "PNG has an alpha channel");
            png.format = PNG_FORMAT_RGBA;
            pixels.resize(PNG_IMAGE_SIZE(png));
            png_image_finish_read(&png, nullptr, pixels.data(), 0, nullptr);
        }
        check(png.width == 2 && png.height == 1, "PNG dimensions");
        check(pixels == std::vector<unsigned char>({10, 20, 30, 40, 200, 100, 50, 0}), "PNG pixels and alpha");


        const std::filesystem::path cropped = std::filesystem::temp_directory_path() / "startool-dds-test-cropped.png";
        check(DdsToPngConverter().convert(input, cropped, error, 1, 0, 1, 1), "DDS crop converts: " + error);
        png_image croppedPng{};
        croppedPng.version = PNG_IMAGE_VERSION;
        std::vector<unsigned char> croppedPixels;
        if (png_image_begin_read_from_file(&croppedPng, cropped.string().c_str())) {
            croppedPng.format = PNG_FORMAT_RGBA;
            croppedPixels.resize(PNG_IMAGE_SIZE(croppedPng));
            png_image_finish_read(&croppedPng, nullptr, croppedPixels.data(), 0, nullptr);
        }
        check(croppedPng.width == 1 && croppedPng.height == 1, "cropped PNG dimensions");
        check(croppedPixels == std::vector<unsigned char>({200, 100, 50, 0}), "cropped PNG pixels and alpha");

        const std::filesystem::path padded = std::filesystem::temp_directory_path() / "startool-dds-test-padded.png";
        std::filesystem::remove(padded);
        check(DdsToPngConverter().convert(input, padded, error, -1, -1, -1, -1, 3), "DDS bottom padding converts: " + error);
        png_image paddedPng{};
        paddedPng.version = PNG_IMAGE_VERSION;
        std::vector<unsigned char> paddedPixels;
        if (png_image_begin_read_from_file(&paddedPng, padded.string().c_str())) {
            paddedPng.format = PNG_FORMAT_RGBA;
            paddedPixels.resize(PNG_IMAGE_SIZE(paddedPng));
            png_image_finish_read(&paddedPng, nullptr, paddedPixels.data(), 0, nullptr);
        }
        check(paddedPng.width == 2 && paddedPng.height == 3, "padded PNG dimensions");
        check(paddedPixels == std::vector<unsigned char>({
            0,0,0,0, 0,0,0,0,
            0,0,0,0, 0,0,0,0,
            10,20,30,40, 200,100,50,0
        }), "padded PNG preserves lower pixels and transparent top");
        std::filesystem::remove(padded);

        const std::filesystem::path invalidPad = std::filesystem::temp_directory_path() / "startool-dds-test-invalid-pad.png";
        std::filesystem::remove(invalidPad);
        check(!DdsToPngConverter().convert(input, invalidPad, error, -1, -1, -1, -1, 0), "invalid bottom align is rejected");
        check(!std::filesystem::exists(invalidPad), "invalid bottom align does not leave output");

        const std::filesystem::path invalid = std::filesystem::temp_directory_path() / "startool-dds-test-invalid.png";
        std::filesystem::remove(invalid);
        check(!DdsToPngConverter().convert(input, invalid, error, 1, 0, 2, 1), "DDS out-of-bounds crop is rejected");
        check(!std::filesystem::exists(invalid), "invalid crop does not leave output");

        std::filesystem::remove(cropped);
        std::filesystem::remove(input);
        std::filesystem::remove(output);
    }

    if (failures == 0) {
        std::cout << "DdsDecoderTest: all checks passed\n";
    }

    return failures == 0 ? 0 : 1;
}
