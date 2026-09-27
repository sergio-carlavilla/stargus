// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ImagesLuaIndexWriter.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{

    bool parseImageId(const std::string &filename, int &id)
    {
        static const std::string prefix = "image_";

        if (
            filename.size() < prefix.size() ||
            filename.compare(0, prefix.size(), prefix) != 0
        ) {
            return false;
        }

        const std::size_t begin = prefix.size();
        const std::size_t end = filename.find('_', begin);

        if (end == std::string::npos || end == begin) {
            return false;
        }

        for (std::size_t i = begin; i < end; ++i) {
            if (!std::isdigit(static_cast<unsigned char>(filename[i]))) {
                return false;
            }
        }

        try {
            id = std::stoi(filename.substr(begin, end - begin));
        } catch (...) {
            return false;
        }

        return true;
    }

}

bool ImagesLuaIndexWriter::write(
    const std::filesystem::path &imagesDirectory,
    std::string &error
) const
{
    std::error_code filesystemError;

    if (!std::filesystem::is_directory(imagesDirectory, filesystemError)) {
        if (filesystemError) {
            error = "Could not inspect images Lua directory: " + filesystemError.message();
        } else {
            error = "Images Lua directory does not exist: " + imagesDirectory.string();
        }

        return false;
    }

    std::vector<std::pair<int, std::string>> images;

    for (
        std::filesystem::directory_iterator iterator(
            imagesDirectory,
            filesystemError
        );
        !filesystemError &&
        iterator != std::filesystem::directory_iterator();
        iterator.increment(filesystemError)
    ) {
        if (!iterator->is_regular_file()) {
            continue;
        }

        const std::filesystem::path path = iterator->path();
        if (path.extension() != ".lua") {
            continue;
        }

        const std::string filename = path.filename().string();
        if (filename == "luagen-images.lua") {
            continue;
        }

        int imageId = -1;

        if (!parseImageId(filename, imageId)) {
            continue;
        }

        images.emplace_back(
            imageId,
            filename
        );
    }

    if (filesystemError) {
        error = "Could not enumerate images Lua directory: " + filesystemError.message();
        return false;
    }

    if (images.empty()) {
        error = "No image Lua files were found in: " + imagesDirectory.string();
        return false;
    }

    std::sort(
        images.begin(),
        images.end(),
        [](
            const auto &left,
            const auto &right
        ) {
            if (left.first != right.first) {
                return left.first < right.first;
            }

            return left.second < right.second;
        }
    );

    const std::filesystem::path output = imagesDirectory / "luagen-images.lua";
    if (std::filesystem::exists(output, filesystemError)) {
        if (filesystemError) {
            error = "Could not inspect images Lua index output: " + filesystemError.message();
        } else {
            error = "Images Lua index already exists: " + output.string();
        }

        return false;
    }

    std::ofstream outputFile(output, std::ios::binary);

    if (!outputFile) {
        error = "Could not open images Lua index for writing: " + output.string();
        return false;
    }

    for (const auto &[imageId, filename] : images) {
        (void)imageId;
        outputFile << "Load(\"luagen/images/" << filename << "\")\n";
    }

    if (!outputFile) {
        outputFile.close();

        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);

        error = "Could not write images Lua index: " + output.string();
        return false;
    }

    outputFile.close();

    return true;
}
