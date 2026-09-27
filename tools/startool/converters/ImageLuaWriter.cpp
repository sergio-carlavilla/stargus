// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ImageLuaWriter.h"

#include <fstream>

bool ImageLuaWriter::write(
    const std::filesystem::path &output,
    const std::string &imageId,
    const std::string &pngRelativePath,
    std::size_t width,
    std::size_t height,
    int numDirections,
    std::string &error
) const
{
    std::ofstream file(output);

    if (!file) {
        error = "Could not open image Lua output: " + output.string();
        return false;
    }

    const std::string fileVariable = imageId + "_file";
    const std::string sizeVariable = imageId + "_size";

    file << fileVariable << " = \"" << pngRelativePath << "\"\n";
    file << sizeVariable << " = {" << width << ", " << height << "}\n";
    file << imageId << "_NumDirections = " << numDirections << "\n";
    file << imageId << " = {\"file\", " << fileVariable << ", \"size\", " << sizeVariable<< "}\n";
    file << imageId << "_var = {File = " << fileVariable << ", Size = " << sizeVariable << "}\n";

    if (!file) {
        error = "Could not write image Lua output: " + output.string();
        return false;
    }

    return true;
}
