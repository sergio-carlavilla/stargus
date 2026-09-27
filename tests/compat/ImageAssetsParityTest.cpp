// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#include <png.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Image {
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    std::vector<unsigned char> pixels;
};

Image loadPng(const fs::path& path)
{
    png_image image{};
    image.version = PNG_IMAGE_VERSION;

    const std::string filename = path.string();

    if (!png_image_begin_read_from_file(&image, filename.c_str())) {
        throw std::runtime_error(
            "Could not open PNG '" + filename + "': " + image.message
        );
    }

    image.format = PNG_FORMAT_RGBA;

    std::vector<unsigned char> pixels(PNG_IMAGE_SIZE(image));

    if (!png_image_finish_read(&image, nullptr, pixels.data(), 0, nullptr)) {

        const std::string error = image.message;
        png_image_free(&image);

        throw std::runtime_error(
            "Could not decode PNG '" + filename + "': " + error
        );
    }

    Image result;
    result.width = image.width;
    result.height = image.height;
    result.pixels = std::move(pixels);

    png_image_free(&image);

    return result;
}

std::string shellQuote(const std::string& value)
{
    std::string result = "'";

    for (const char c : value) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }

    result += '\'';

    return result;
}

bool comparePng(
    const fs::path& referencePath,
    const fs::path& candidatePath)
{
    if (!fs::exists(referencePath)) {
        std::cerr << "FAIL: StarTool legacy tool reference does not exist:\n" << "  " << referencePath << '\n';
        return false;
    }

    if (!fs::exists(candidatePath)) {
        std::cerr << "FAIL: StarTool output does not exist:\n" << "  " << candidatePath << '\n';
        return false;
    }

    const Image reference = loadPng(referencePath);
    const Image candidate = loadPng(candidatePath);

    if (reference.width != candidate.width || reference.height != candidate.height) {

        std::cerr
            << "FAIL: image dimensions differ\n"
            << "  StarTool legacy: "
            << reference.width << 'x' << reference.height << '\n'
            << "  StarTool: "
            << candidate.width << 'x' << candidate.height << '\n';

        return false;
    }

    if (reference.pixels.size() != candidate.pixels.size()) {
        std::cerr << "FAIL: decoded pixel buffer sizes differ\n";
        return false;
    }

    for (std::size_t i = 0; i < reference.pixels.size(); i += 4) {
        const bool equal =
            reference.pixels[i + 0] == candidate.pixels[i + 0] &&
            reference.pixels[i + 1] == candidate.pixels[i + 1] &&
            reference.pixels[i + 2] == candidate.pixels[i + 2] &&
            reference.pixels[i + 3] == candidate.pixels[i + 3];

        if (!equal) {
            const std::size_t pixel = i / 4;
            const std::size_t x = pixel % reference.width;
            const std::size_t y = pixel / reference.width;

            std::cerr
                << "FAIL: first different pixel at "
                << x << ',' << y << '\n'
                << "  StarTool legacy RGBA: "
                << static_cast<int>(reference.pixels[i + 0]) << ','
                << static_cast<int>(reference.pixels[i + 1]) << ','
                << static_cast<int>(reference.pixels[i + 2]) << ','
                << static_cast<int>(reference.pixels[i + 3]) << '\n'
                << "  StarTool4 RGBA: "
                << static_cast<int>(candidate.pixels[i + 0]) << ','
                << static_cast<int>(candidate.pixels[i + 1]) << ','
                << static_cast<int>(candidate.pixels[i + 2]) << ','
                << static_cast<int>(candidate.pixels[i + 3]) << '\n';

            return false;
        }
    }

    return true;
}

}

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::cerr
            << "Usage:\n"
            << "  ImageAssetsParityTest "
            << "<startool4> "
            << "<starcraft-dir> "
            << "<manifest> "
            << "<startool3-reference-dir>\n";

        return EXIT_FAILURE;
    }

    const fs::path startool4 = argv[1];
    const fs::path starcraftDir = argv[2];
    const fs::path manifest = argv[3];
    const fs::path referenceDir = argv[4];

    const fs::path outputDir =
        fs::temp_directory_path() /
        "startool4-image-assets-parity";

    fs::remove_all(outputDir);
    fs::create_directories(outputDir);

    const std::string command =
        shellQuote(startool4.string()) +
        " import " +
        shellQuote(starcraftDir.string()) +
        " " +
        shellQuote(manifest.string()) +
        " " +
        shellQuote(outputDir.string());

    std::cout << "Running StarTool4 import...\n";

    const int result = std::system(command.c_str());

    if (result != 0) {
        std::cerr << "FAIL: StarTool import returned " << result << '\n';
        return EXIT_FAILURE;
    }

    constexpr std::array<std::string_view, 18> assets = {
        "graphics/unit/thingy/ensgoom.png",
        "graphics/unit/thingy/plasma_gfire.png",
        "graphics/unit/thingy/flamer_ofire.png",
        "graphics/unit/thingy/pshield_bfire.png",
        "graphics/unit/thingy/elbbat_ofire.png",
        "graphics/unit/thingy/tileset/jungle/dd091.png",
        "graphics/unit/thingy/tbangl_bexpl.png",
        "graphics/unit/thingy/tbangl_ofire.png",

        "graphics/unit/zerg/avenger.png",
        "graphics/unit/zerg/zavbirth.png",
        "graphics/unit/zerg/zavdeath.png",
        "graphics/unit/zerg/zavexplo.png",
        "graphics/unit/zerg/brood.png",
        "graphics/unit/zerg/zbrdeath.png",
        "graphics/unit/zerg/bugguy.png",
        "graphics/unit/thingy/zbgexplo_gfire.png",
        "graphics/unit/zerg/cocoon.png",
        "graphics/unit/zerg/defiler.png"
    };

    std::size_t checked = 0;
    std::size_t failed = 0;

    for (const std::string_view asset : assets) {
        const fs::path relativePath{asset};

        std::cout << "Checking: " << relativePath << '\n';

        try {
            if (!comparePng(referenceDir / relativePath, outputDir / relativePath)) {
                ++failed;
            } else {
                std::cout << "  IDENTICAL\n";
            }
        } catch (const std::exception& exception) {
            std::cerr << "FAIL: " << exception.what() << '\n';
            ++failed;
        }

        ++checked;
    }

    std::cout
        << "\nChecked: " << checked
        << "\nFailed:  " << failed
        << '\n';

    if (failed != 0) {
        std::cerr << "IMAGE ASSETS PARITY FAILED\n";
        return EXIT_FAILURE;
    }

    std::cout << "ALL IMAGE ASSETS IDENTICAL TO STARTOOL3\n";

    fs::remove_all(outputDir);

    return EXIT_SUCCESS;
}
