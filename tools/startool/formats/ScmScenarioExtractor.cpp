// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ScmScenarioExtractor.h"

#include <StormLib.h>

#include <filesystem>
#include <string>
#include <system_error>

namespace
{

    constexpr const char *ScenarioPath = "staredit\\scenario.chk";

}

bool ScmScenarioExtractor::extract(
    const std::filesystem::path &scm,
    const std::filesystem::path &output,
    std::string &error
) const
{
    std::error_code filesystemError;

    const bool scmExists = std::filesystem::exists(
        scm,
        filesystemError
    );

    if (filesystemError) {
        error = "Could not inspect SCM file '" + scm.string() + "': " + filesystemError.message();
        return false;
    }

    if (!scmExists) {
        error = "SCM file does not exist: " + scm.string();
        return false;
    }

    const bool scmIsFile = std::filesystem::is_regular_file(
        scm,
        filesystemError
    );

    if (filesystemError) {
        error = "Could not inspect SCM file '" + scm.string() + "': " + filesystemError.message();
        return false;
    }

    if (!scmIsFile) {
        error = "SCM path is not a regular file: " + scm.string();
        return false;
    }

    const bool outputExists = std::filesystem::exists(
        output,
        filesystemError
    );

    if (filesystemError) {
        error = "Could not inspect CHK output '" + output.string() + "': " + filesystemError.message();
        return false;
    }

    if (outputExists) {
        error = "CHK output already exists: " + output.string();
        return false;
    }

    const std::filesystem::path parentDirectory = output.parent_path();

    if (!parentDirectory.empty()) {
        std::filesystem::create_directories(
            parentDirectory,
            filesystemError
        );

        if (filesystemError) {
            error = "Could not create CHK output directory: " + filesystemError.message();
            return false;
        }
    }

    HANDLE archive = nullptr;

    if (!SFileOpenArchive(
        scm.string().c_str(),
        0,
        STREAM_FLAG_READ_ONLY,
        &archive
    )) {
        error = "Could not open SCM archive: " + scm.string();
        return false;
    }

    const bool extracted = SFileExtractFile(
        archive,
        ScenarioPath,
        output.string().c_str(),
        SFILE_OPEN_FROM_MPQ
    );

    SFileCloseArchive(archive);

    if (!extracted) {
        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);

        error = "Could not extract '" + std::string(ScenarioPath) + "' from SCM archive: " + scm.string();
        return false;
    }

    const bool extractedFileExists = std::filesystem::is_regular_file(
        output,
        filesystemError
    );

    if (filesystemError || !extractedFileExists) {
        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);

        if (filesystemError) {
            error = "Could not inspect extracted CHK file: " + filesystemError.message();
        } else {
            error = "SCM extraction did not produce a CHK file: " + output.string();
        }

        return false;
    }

    return true;
}
