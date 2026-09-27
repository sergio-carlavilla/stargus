// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SmkToMngConverter.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace
{

    std::string quote(const std::filesystem::path &path)
    {
        return "\"" + path.string() + "\"";
    }

}

bool SmkToMngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    std::string &error
) const
{
    std::filesystem::path base = output;
    base.replace_extension();

    const std::filesystem::path temporarySmk = std::filesystem::path(base.string() + ".smk");
    const std::filesystem::path framesDirectory = std::filesystem::path(base.string() + "_png");

    std::error_code filesystemError;

    std::filesystem::copy_file( input, temporarySmk, std::filesystem::copy_options::overwrite_existing, filesystemError);
    if (filesystemError) {
        error = "Could not create temporary SMK file: " + filesystemError.message();
        return false;
    }

    std::filesystem::create_directories(
        framesDirectory,
        filesystemError
    );

    if (filesystemError) {
        std::error_code cleanupError;
        std::filesystem::remove(temporarySmk, cleanupError);
        error = "Could not create temporary portrait frame directory: " + filesystemError.message();
        return false;
    }

    const std::filesystem::path framePattern = framesDirectory / "image%05d.png";
    const std::string ffmpegCommand = "ffmpeg -y -i " + quote(temporarySmk) + " -codec:v png -qscale:v 31" " -pix_fmt yuv420p " + quote(framePattern);
    const int ffmpegResult = std::system(ffmpegCommand.c_str());

    bool converted = false;

    if (ffmpegResult == 0) {
        converted = convertFramesToMng(framesDirectory, output);
    }

    filesystemError.clear();

    std::filesystem::remove(temporarySmk, filesystemError);

    std::error_code directoryCleanupError;

    std::filesystem::remove_all(framesDirectory, directoryCleanupError);

    if (ffmpegResult != 0) {
        std::error_code outputCleanupError;
        std::filesystem::remove(output, outputCleanupError);
        error = "FFmpeg failed while extracting portrait PNG frames";
        return false;
    }

    if (!converted) {
        std::error_code outputCleanupError;
        std::filesystem::remove(output, outputCleanupError);
        error = "ImageMagick/GraphicsMagick failed while converting portrait frames to MNG";
        return false;
    }

    if (filesystemError) {
        error = "Could not remove temporary SMK file: " + filesystemError.message();
        return false;
    }

    if (directoryCleanupError) {
        error = "Could not remove temporary portrait frame directory: " + directoryCleanupError.message();
        return false;
    }

    return true;
}

bool SmkToMngConverter::convertFramesToMng(
    const std::filesystem::path &framesDirectory,
    const std::filesystem::path &output
) const
{
    // Startool legacy deliberately quotes the wildcard and lets
    // ImageMagick / GraphicsMagick expand it itself
    const std::filesystem::path frameGlob = framesDirectory / "image*.png";
    const std::string convertCommand = "convert " + quote(frameGlob) + " -delay 4 " + quote(output);
    const std::string imageMagick7Command = "magick " + convertCommand;

    if (std::system(imageMagick7Command.c_str()) == 0) {
        return true;
    }

    if (std::system(convertCommand.c_str()) == 0) {
        return true;
    }

    const std::string graphicsMagickCommand = "gm " + convertCommand;

    return std::system(graphicsMagickCommand.c_str()) == 0;
}
