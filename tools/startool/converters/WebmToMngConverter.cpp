// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "WebmToMngConverter.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>

namespace
{

    std::string quote(const std::filesystem::path &path)
    {
        return "\"" + path.string() + "\"";
    }

    /*
     * FFmpeg's native VP8/VP9 decoders ignore the WebM alpha plane, so the
     * matching libvpx decoder must be forced. The Matroska CodecID is stored
     * in the Tracks element at the start of the file
     */
    std::optional<std::string> alphaDecoder(const std::filesystem::path &input)
    {
        std::ifstream stream(input, std::ios::binary);

        if (!stream) {
            return std::nullopt;
        }

        std::string header(64 * 1024, '\0');
        stream.read(header.data(), static_cast<std::streamsize>(header.size()));
        header.resize(static_cast<std::size_t>(stream.gcount()));

        if (header.find("V_VP8") != std::string::npos) {
            return "libvpx";
        }

        if (header.find("V_VP9") != std::string::npos) {
            return "libvpx-vp9";
        }

        return std::nullopt;
    }

}

bool WebmToMngConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    std::string &error
) const
{
    std::error_code filesystemError;

    if (!std::filesystem::is_regular_file(input, filesystemError)) {
        error = "WebM input does not exist: " + input.string();
        return false;
    }

    const std::optional<std::string> decoder = alphaDecoder(input);

    if (!decoder) {
        error = "Unsupported WebM video codec, expected VP8 or VP9: " + input.string();
        return false;
    }

    std::filesystem::path base = output;
    base.replace_extension();

    const std::filesystem::path framesDirectory = std::filesystem::path(base.string() + "_png");

    std::filesystem::create_directories(
        framesDirectory,
        filesystemError
    );

    if (filesystemError) {
        error = "Could not create temporary portrait frame directory: " + filesystemError.message();
        return false;
    }

    // RGBA frames keep the transparency decoded from the WebM alpha plane
    const std::filesystem::path framePattern = framesDirectory / "image%05d.png";
    const std::string ffmpegCommand = "ffmpeg -y -codec:v " + *decoder + " -i " + quote(input) + " -pix_fmt rgba " + quote(framePattern);
    const int ffmpegResult = std::system(ffmpegCommand.c_str());

    bool converted = false;

    if (ffmpegResult == 0) {
        converted = convertFramesToMng(framesDirectory, output);
    }

    std::error_code directoryCleanupError;

    std::filesystem::remove_all(framesDirectory, directoryCleanupError);

    if (ffmpegResult != 0) {
        std::error_code outputCleanupError;
        std::filesystem::remove(output, outputCleanupError);
        error = "FFmpeg failed while extracting WebM PNG frames";
        return false;
    }

    if (!converted) {
        std::error_code outputCleanupError;
        std::filesystem::remove(output, outputCleanupError);
        error = "ImageMagick/GraphicsMagick failed while converting portrait frames to MNG";
        return false;
    }

    if (directoryCleanupError) {
        error = "Could not remove temporary portrait frame directory: " + directoryCleanupError.message();
        return false;
    }

    return true;
}

bool WebmToMngConverter::convertFramesToMng(
    const std::filesystem::path &framesDirectory,
    const std::filesystem::path &output
) const
{
    // Same frame assembly and timing as SmkToMngConverter
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
