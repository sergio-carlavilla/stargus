// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ImportExecutor.h"

#include "SourceReader.h"
#include "converters/GrpToPngConverter.h"
#include "converters/PcxToPngConverter.h"
#include "converters/WavToOggConverter.h"

#include <system_error>
#include <variant>

namespace
{

    void removeEmptyDirectories(std::filesystem::path directory, const std::filesystem::path &root)
    {
        std::error_code error;

        while (!directory.empty()) {
            const bool removed = std::filesystem::remove(
                directory,
                error
            );

            if (error || !removed) {
                return;
            }

            if (directory == root) {
                return;
            }

            directory = directory.parent_path();
        }
    }

}

bool ImportExecutor::stage(
    const ImportTask &task,
    const SourceReaderRegistry &readers,
    const std::filesystem::path &stagingRoot,
    std::filesystem::path &stagedPath,
    std::string &error
) const
{
    const SourceReader *reader = readers.find(task.source);
    if (reader == nullptr) {
        error =
            "No source reader registered for task '" +
            task.id +
            "': " +
            task.source;

        return false;
    }

    if (!reader->contains(task.input)) {
        error =
            "Resource not found for task '" +
            task.id +
            "': " +
            task.input;

        return false;
    }

    const std::filesystem::path destination = stagingRoot / std::filesystem::path(task.input);

    std::error_code filesystemError;
    if (std::filesystem::exists(destination, filesystemError)) {
        if (filesystemError) {
            error =
                "Could not inspect staging destination for task '" +
                task.id +
                "': " +
                filesystemError.message();

            return false;
        }

        error =
            "Staging destination already exists for task '" +
            task.id +
            "': " +
            destination.string();

        return false;
    }

    const std::filesystem::path parentDirectory = destination.parent_path();
    if (!parentDirectory.empty()) {
        std::filesystem::create_directories(parentDirectory, filesystemError);

        if (filesystemError) {
            error =
                "Could not create staging directory for task '" +
                task.id +
                "': " +
                filesystemError.message();

            return false;
        }
    }

    if (!reader->extract(task.input, destination)) {
        std::error_code cleanupError;
        std::filesystem::remove(destination, cleanupError);
        removeEmptyDirectories(parentDirectory, stagingRoot);

        error =
            "Could not stage resource for task '" +
            task.id +
            "': " +
            task.input;

        return false;
    }

    stagedPath = destination;

    return true;
}

bool ImportExecutor::execute(
    const ImportTask &task,
    const SourceReaderRegistry &readers,
    const std::map<std::string, LoadedPalette> &palettes,
    const std::filesystem::path &destinationRoot,
    std::filesystem::path &outputPath,
    std::string &error
) const
{
    const std::filesystem::path destination = destinationRoot / std::filesystem::path(task.output);

    std::error_code filesystemError;
    if (std::filesystem::exists(destination, filesystemError)) {
        if (filesystemError) {
            error =
                "Could not inspect output destination for task '" +
                task.id +
                "': " +
                filesystemError.message();

            return false;
        }

        error =
            "Output destination already exists for task '" +
            task.id +
            "': " +
            destination.string();

        return false;
    }

    const std::filesystem::path parentDirectory = destination.parent_path();
    if (!parentDirectory.empty()) {
        std::filesystem::create_directories(parentDirectory, filesystemError);

        if (filesystemError) {
            error =
                "Could not create output directory for task '" +
                task.id +
                "': " +
                filesystemError.message();

            return false;
        }
    }

    if (task.operation == ManifestOperation::Extract) {
        const SourceReader *reader = readers.find(task.source);

        if (reader == nullptr) {
            error =
                "No source reader registered for task '" +
                task.id +
                "': " +
                task.source;

            return false;
        }

        if (!reader->contains(task.input)) {
            error =
                "Resource not found for task '" +
                task.id +
                "': " +
                task.input;

            return false;
        }

        if (!reader->extract(task.input, destination)){
            std::error_code cleanupError;
            std::filesystem::remove(destination, cleanupError);

            error =
                "Could not extract resource for task '" +
                task.id +
                "': " +
                task.input;

            return false;
        }

        outputPath = destination;

        return true;
    }

    const std::filesystem::path stagingRoot = destinationRoot / ".startool4-staging";
    std::filesystem::path stagedPath;

    if (!stage(task, readers, stagingRoot, stagedPath, error)) {
        return false;
    }

    bool converted = false;

    switch (task.operation) {
        case ManifestOperation::Extract:
            break;

        case ManifestOperation::GrpToPng: {
            const auto palette = palettes.find(task.palette);

            if (palette == palettes.end()) {
                error =
                    "Palette not loaded for task '" +
                    task.id +
                    "': " +
                    task.palette;

                break;
            }

            GrpToPngConverter converter;
            converted = std::visit(
                [&](const auto &loadedPalette)
                {
                    return converter.convert(
                        stagedPath,
                        destination,
                        loadedPalette,
                        task.rgba,
                        error
                    );
                },
                palette->second
            );
            break;
        }

        case ManifestOperation::PcxToPng: {
            PcxToPngConverter converter;
            converted = converter.convert(stagedPath, destination, error);
            break;
        }

        case ManifestOperation::WavToOgg: {
            WavToOggConverter converter;
            converted = converter.convert(stagedPath, destination, error);
            break;
        }
    }

    std::filesystem::remove(stagedPath, filesystemError);

    if (filesystemError) {
        error =
            "Could not remove staged resource for task '" +
            task.id +
            "': " +
            filesystemError.message();

        return false;
    }

    removeEmptyDirectories(stagedPath.parent_path(), stagingRoot);

    if (!converted) {
        std::error_code cleanupError;
        std::filesystem::remove(destination, cleanupError);
        return false;
    }

    outputPath = destination;

    return true;
}
