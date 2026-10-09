// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "WebmToOgvConverter.h"

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

bool WebmToOgvConverter::convert(
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

    /*
     * Same Theora/Vorbis output settings as SmkToOgvConverter, so imported
     * videos are equivalent regardless of the source container
     */
    const std::string command = "ffmpeg -y -i " + quote(input) + " -codec:v libtheora -qscale:v 31" " -codec:a libvorbis -qscale:a 15" " -pix_fmt yuv420p " + quote(output);
    const int result = std::system(command.c_str());

    if (result != 0) {
        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);
        error = "FFmpeg failed while converting WebM to OGV";
        return false;
    }

    return true;
}
