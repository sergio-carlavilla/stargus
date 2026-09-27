// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SmkToOgvConverter.h"

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

bool SmkToOgvConverter::convert(
    const std::filesystem::path &input,
    const std::filesystem::path &output,
    std::string &error
) const
{
    std::filesystem::path base = output;
    base.replace_extension();

    const std::filesystem::path temporarySmk = std::filesystem::path(base.string() + ".smk");

    std::error_code filesystemError;

    std::filesystem::copy_file(input, temporarySmk, std::filesystem::copy_options::overwrite_existing, filesystemError);
    if (filesystemError) {
        error = "Could not create temporary SMK file: " + filesystemError.message();
        return false;
    }

    const std::string command = "ffmpeg -y -i " + quote(temporarySmk) + " -codec:v libtheora -qscale:v 31" " -codec:a libvorbis -qscale:a 15" " -pix_fmt yuv420p " + quote(output);
    const int result = std::system(command.c_str());

    filesystemError.clear();

    std::filesystem::remove(temporarySmk, filesystemError);

    if (result != 0) {
        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);
        error = "FFmpeg failed while converting SMK to OGV";
        return false;
    }

    if (filesystemError) {
        error = "Could not remove temporary SMK file: " + filesystemError.message();
        return false;
    }

    return true;
}
