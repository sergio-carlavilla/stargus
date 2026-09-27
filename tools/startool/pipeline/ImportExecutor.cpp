// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ImportExecutor.h"

#include "SourceReader.h"
#include "converters/GrpFrameToPngConverter.h"
#include "converters/GrpFramesToPngConverter.h"
#include "converters/GrpToPngConverter.h"
#include "converters/ImageAssetResolver.h"
#include "converters/ImageLuaWriter.h"
#include "converters/PcxToPngConverter.h"
#include "converters/TilesetToLuaConverter.h"
#include "converters/TilesetToPngConverter.h"
#include "converters/WavToOggConverter.h"
#include "formats/GrpDecoder.h"

#include <algorithm>
#include <system_error>
#include <variant>
#include <vector>

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
        error = "No source reader registered for task '" + task.id + "': " + task.source;
        return false;
    }

    if (!reader->contains(task.input)) {
        error = "Resource not found for task '" + task.id + "': " + task.input;
        return false;
    }

    const std::filesystem::path destination = stagingRoot / std::filesystem::path(task.input);

    std::error_code filesystemError;
    if (std::filesystem::exists(destination, filesystemError)) {
        if (filesystemError) {
            error = "Could not inspect staging destination for task '" + task.id + "': " + filesystemError.message();
            return false;
        }

        error = "Staging destination already exists for task '" + task.id + "': " + destination.string();
        return false;
    }

    const std::filesystem::path parentDirectory = destination.parent_path();
    if (!parentDirectory.empty()) {
        std::filesystem::create_directories(parentDirectory, filesystemError);

        if (filesystemError) {
            error = "Could not create staging directory for task '" + task.id + "': " + filesystemError.message();
            return false;
        }
    }

    if (!reader->extract(task.input, destination)) {
        std::error_code cleanupError;
        std::filesystem::remove(destination, cleanupError);
        removeEmptyDirectories(parentDirectory, stagingRoot);

        error = "Could not stage resource for task '" + task.id + "': " + task.input;
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
    if (task.operation == ManifestOperation::ImageAssets) {
        const SourceReader *reader = readers.find(task.source);

        if (reader == nullptr) {
            error = "No source reader registered for task '" + task.id + "': " + task.source;
            return false;
        }

        const std::filesystem::path stagingRoot = destinationRoot / ".startool4-staging";

        ImportTask tableTask = task;
        tableTask.input = task.table;

        std::filesystem::path stagedImagesDat;
        std::filesystem::path stagedImagesTbl;
        std::filesystem::path stagedGrp;

        auto cleanupStaged = [&]()
        {
            std::error_code cleanupError;

            if (!stagedGrp.empty()) {
                std::filesystem::remove(stagedGrp, cleanupError);
                removeEmptyDirectories(stagedGrp.parent_path(), stagingRoot);
            }

            cleanupError.clear();

            if (!stagedImagesTbl.empty()) {
                std::filesystem::remove(stagedImagesTbl, cleanupError);
                removeEmptyDirectories(stagedImagesTbl.parent_path(), stagingRoot);
            }

            cleanupError.clear();

            if (!stagedImagesDat.empty()) {
                std::filesystem::remove(stagedImagesDat, cleanupError);
                removeEmptyDirectories(stagedImagesDat.parent_path(), stagingRoot);
            }
        };

        if (!stage(task, readers, stagingRoot, stagedImagesDat, error)) {
            return false;
        }

        if (!stage(tableTask, readers, stagingRoot, stagedImagesTbl, error)) {
            cleanupStaged();
            return false;
        }

        ImageAssetMetadata metadata;
        ImageAssetResolver resolver;

        if (!resolver.resolve(stagedImagesDat, stagedImagesTbl, static_cast<std::size_t>(task.image), metadata, error)) {
            cleanupStaged();
            return false;
        }

        if (!metadata.save) {
            error = "Image " + std::to_string(task.image) + " is intentionally not exported by Startool legacy";

            cleanupStaged();
            return false;
        }

        const auto loadedPalette = palettes.find(metadata.palette);
        if (loadedPalette == palettes.end()) {
            error = "Palette not loaded for image " + std::to_string(task.image) + ": " + metadata.palette;

            cleanupStaged();
            return false;
        }

        if (!reader->contains(metadata.grpInput)) {
            error = "Resolved GRP for image " + std::to_string(task.image) + " is missing: " + metadata.grpInput;

            cleanupStaged();
            return false;
        }

        ImportTask grpTask = task;
        grpTask.input = metadata.grpInput;

        if (!stage(grpTask, readers, stagingRoot, stagedGrp, error)) {
            cleanupStaged();
            return false;
        }

        const std::filesystem::path pngDestination = destinationRoot / std::filesystem::path(metadata.pngOutput);
        const std::filesystem::path luaDestination = destinationRoot / std::filesystem::path(metadata.luaOutput);

        std::error_code filesystemError;
        const bool pngExists = std::filesystem::exists(pngDestination, filesystemError);

        if (filesystemError) {
            error = "Could not inspect image PNG destination for task '" + task.id + "': " + filesystemError.message();
            cleanupStaged();
            return false;
        }

        filesystemError.clear();
        const bool luaExists = std::filesystem::exists(luaDestination, filesystemError);

        if (filesystemError) {
            error = "Could not inspect image Lua destination for task '" + task.id + "': " + filesystemError.message();
            cleanupStaged();
            return false;
        }

        if (luaExists) {
            error = "Image Lua output destination already exists for task '" + task.id + "': " + luaDestination.string();
            cleanupStaged();
            return false;
        }

        std::filesystem::create_directories(pngDestination.parent_path(), filesystemError);
        if (filesystemError) {
            error = "Could not create image PNG output directory: " + filesystemError.message();
            cleanupStaged();
            return false;
        }

        std::filesystem::create_directories(luaDestination.parent_path(), filesystemError);
        if (filesystemError) {
            error = "Could not create image Lua output directory: " + filesystemError.message();
            cleanupStaged();
            return false;
        }

        GrpDecoder grpDecoder;
        GrpImage grpImage;

        if (!grpDecoder.decode(stagedGrp, grpImage, error)) {
            cleanupStaged();
            return false;
        }

        std::size_t tileWidth = grpImage.maximumWidth;
        std::size_t tileHeight = grpImage.maximumHeight;

        if (grpImage.uncompressed) {
            tileWidth = 0;
            tileHeight = 0;

            for (const GrpFrame &frame : grpImage.frames) {
                tileWidth = std::max(
                    tileWidth,
                    static_cast<std::size_t>(frame.xOffset) +
                    static_cast<std::size_t>(frame.width)
                );

                tileHeight = std::max(
                    tileHeight,
                    static_cast<std::size_t>(frame.yOffset) +
                    static_cast<std::size_t>(frame.height)
                );
            }
        }

        GrpToPngConverter grpConverter;
        const bool pngConverted = std::visit(
            [&](const auto &palette)
            {
                return grpConverter.convert(
                    stagedGrp,
                    pngDestination,
                    palette,
                    metadata.rgba,
                    error
                );
            },
            loadedPalette->second
        );

        if (!pngConverted) {
            if (!pngExists) {
                std::error_code cleanupError;
                std::filesystem::remove(pngDestination, cleanupError);
            }

            cleanupStaged();
            return false;
        }

        ImageLuaWriter luaWriter;
        const bool luaWritten = luaWriter.write(
            luaDestination,
            metadata.luaId,
            metadata.pngOutput,
            tileWidth,
            tileHeight,
            metadata.gfxTurns ? 32 : 1,
            error
        );

        if (!luaWritten) {
            std::error_code cleanupError;

            if (!pngExists) {
                std::filesystem::remove(pngDestination, cleanupError);
                cleanupError.clear();
            }

            std::filesystem::remove(luaDestination, cleanupError);
            cleanupStaged();
            return false;
        }

        cleanupStaged();

        outputPath = pngDestination;
        return true;
    }

    const std::filesystem::path destination = destinationRoot / std::filesystem::path(task.output);

    std::error_code filesystemError;
    if (std::filesystem::exists(destination, filesystemError)) {
        if (filesystemError) {
            error = "Could not inspect output destination for task '" + task.id + "': " + filesystemError.message();
            return false;
        }

        error = "Output destination already exists for task '" + task.id + "': " + destination.string();
        return false;
    }

    const std::filesystem::path parentDirectory = destination.parent_path();
    if (!parentDirectory.empty()) {
        std::filesystem::create_directories(parentDirectory, filesystemError);

        if (filesystemError) {
            error = "Could not create output directory for task '" + task.id + "': " + filesystemError.message();
            return false;
        }
    }

    if (task.operation == ManifestOperation::Extract) {
        const SourceReader *reader = readers.find(task.source);

        if (reader == nullptr) {
            error = "No source reader registered for task '" + task.id + "': " + task.source;
            return false;
        }

        if (!reader->contains(task.input)) {
            error = "Resource not found for task '" + task.id + "': " + task.input;
            return false;
        }

        if (!reader->extract(task.input, destination)){
            std::error_code cleanupError;
            std::filesystem::remove(destination, cleanupError);

            error = "Could not extract resource for task '" + task.id + "': " + task.input;
            return false;
        }

        outputPath = destination;

        return true;
    }

    if (task.operation == ManifestOperation::TilesetToLua) {
        const SourceReader *reader = readers.find(task.source);

        if (reader == nullptr) {
            error = "No source reader registered for task '" + task.id + "': " + task.source;
            return false;
        }

        const std::filesystem::path stagingRoot = destinationRoot / ".startool4-staging";

        ImportTask cv5Task = task;
        cv5Task.input += ".cv5";

        ImportTask vf4Task = task;
        vf4Task.input += ".vf4";

        std::filesystem::path stagedCv5;
        std::filesystem::path stagedVf4;

        if (!stage(cv5Task, readers, stagingRoot, stagedCv5, error)) {
            return false;
        }

        if (!stage(vf4Task, readers, stagingRoot, stagedVf4, error)) {
            std::error_code cleanupError;
            std::filesystem::remove(stagedCv5, cleanupError);
            removeEmptyDirectories(stagedCv5.parent_path(), stagingRoot);
            return false;
        }

        const std::string tilesetName = destination.stem().string();
        const std::string image = "tilesets/" + tilesetName + "/" + tilesetName + ".png";

        TilesetToLuaConverter converter;
        const bool converted = converter.convert(
            stagedCv5,
            stagedVf4,
            destination,
            tilesetName,
            image,
            error
        );

        std::error_code cleanupError;
        std::filesystem::remove(stagedCv5, cleanupError);
        cleanupError.clear();
        std::filesystem::remove(stagedVf4, cleanupError);
        removeEmptyDirectories(stagedCv5.parent_path(), stagingRoot);

        if (!converted) {
            std::filesystem::remove(destination, cleanupError);
            return false;
        }

        outputPath = destination;
        return true;
    }

    if (task.operation == ManifestOperation::TilesetToPng) {
        const SourceReader *reader = readers.find(task.source);

        if (reader == nullptr) {
            error = "No source reader registered for task '" + task.id + "': " + task.source;
            return false;
        }

        const auto loadedPalette = palettes.find(task.palette);
        if (loadedPalette == palettes.end()) {
            error = "Palette not loaded for task '" + task.id + "': " + task.palette;
            return false;
        }

        const Palette *palette = std::get_if<Palette>(&loadedPalette->second);
        if (palette == nullptr) {
            error = "Tileset conversion requires a one-dimensional palette for task '" + task.id + "': " + task.palette;
            return false;
        }

        const std::filesystem::path stagingRoot = destinationRoot / ".startool4-staging";

        ImportTask vx4Task = task;
        vx4Task.input += ".vx4";

        ImportTask vr4Task = task;
        vr4Task.input += ".vr4";

        std::filesystem::path stagedVx4;
        std::filesystem::path stagedVr4;

        if (!stage(vx4Task, readers, stagingRoot, stagedVx4, error)) {
            return false;
        }

        if (!stage(vr4Task, readers, stagingRoot, stagedVr4, error)) {
            std::error_code cleanupError;
            std::filesystem::remove(stagedVx4, cleanupError);
            removeEmptyDirectories(stagedVx4.parent_path(), stagingRoot);
            return false;
        }

        TilesetToPngConverter converter;
        const bool converted = converter.convert(
            stagedVx4,
            stagedVr4,
            destination,
            *palette,
            error
        );

        std::error_code cleanupError;
        std::filesystem::remove(stagedVx4, cleanupError);
        cleanupError.clear();
        std::filesystem::remove(stagedVr4, cleanupError);
        removeEmptyDirectories(stagedVx4.parent_path(), stagingRoot);

        if (!converted) {
            std::filesystem::remove(destination, cleanupError);
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

        case ManifestOperation::GrpFrameToPng: {
            const auto palette = palettes.find(task.palette);

            if (palette == palettes.end()) {
                error = "Palette not loaded for task '" + task.id + "': " + task.palette;
                break;
            }

            const Palette *palette1D = std::get_if<Palette>(&palette->second);
            if (palette1D == nullptr) {
                error = "GRP frame conversion requires a one-dimensional palette for task '" + task.id + "': " + task.palette;
                break;
            }

            GrpFrameToPngConverter converter;
            converted = converter.convert(
                stagedPath,
                destination,
                *palette1D,
                static_cast<std::size_t>(task.frame),
                task.rgba,
                error
            );
            break;
        }

        case ManifestOperation::GrpFramesToPng: {
            const auto palette = palettes.find(task.palette);

            if (palette == palettes.end()) {
                error = "Palette not loaded for task '" + task.id + "': " + task.palette;
                break;
            }

            const Palette *palette1D = std::get_if<Palette>(&palette->second);
            if (palette1D == nullptr) {
                error = "GRP frames conversion requires a one-dimensional palette for task '" + task.id + "': " + task.palette;
                break;
            }

            std::vector<std::size_t> frameIndices;
            frameIndices.reserve(task.frames.size());

            for (const int frame : task.frames) {
                frameIndices.push_back(static_cast<std::size_t>(frame));
            }

            GrpFramesToPngConverter converter;
            converted = converter.convert(
                stagedPath,
                destination,
                *palette1D,
                frameIndices,
                task.rgba,
                error
            );
            break;
        }

        case ManifestOperation::GrpToPng: {
            const auto palette = palettes.find(task.palette);

            if (palette == palettes.end()) {
                error = "Palette not loaded for task '" + task.id + "': " + task.palette;
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

        case ManifestOperation::ImageAssets:
            break;

        case ManifestOperation::PcxToPng: {
            PcxToPngConverter converter;
            converted = converter.convert(stagedPath, destination, error);
            break;
        }

        case ManifestOperation::TilesetToLua:
        case ManifestOperation::TilesetToPng:
            break;

        case ManifestOperation::WavToOgg: {
            WavToOggConverter converter;
            converted = converter.convert(stagedPath, destination, error);
            break;
        }
    }

    std::filesystem::remove(stagedPath, filesystemError);

    if (filesystemError) {
        error = "Could not remove staged resource for task '" + task.id + "': " + filesystemError.message();
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
