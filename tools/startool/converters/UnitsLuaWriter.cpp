// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "UnitsLuaWriter.h"

#include "UnitLuaWriter.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace
{

    void cleanupGeneratedFiles(
        const std::vector<std::filesystem::path> &generatedFiles
    )
    {
        std::error_code error;

        for (auto iterator = generatedFiles.rbegin(); iterator != generatedFiles.rend(); ++iterator) {
            error.clear();
            std::filesystem::remove(*iterator, error);
        }
    }

}

bool UnitsLuaWriter::write(
    const std::vector<UnitDefinition> &units,
    UnitDataResolver &resolver,
    const std::filesystem::path &output,
    std::string &error
) const
{
    if (units.empty()) {
        error = "Units Lua generation requires at least one unit definition";
        return false;
    }

    std::error_code filesystemError;

    if (std::filesystem::exists(output, filesystemError)) {
        if (filesystemError) {
            error = "Could not inspect units Lua output: " + filesystemError.message();
            return false;
        }

        error = "Units Lua output already exists: " + output.string();
        return false;
    }

    const std::filesystem::path outputDirectory = output.parent_path();

    if (!outputDirectory.empty()) {
        std::filesystem::create_directories(outputDirectory, filesystemError);

        if (filesystemError) {
            error = "Could not create units Lua output directory: " + filesystemError.message();
            return false;
        }
    }

    std::vector<std::filesystem::path> generatedFiles;

    std::set<int> unitIds;
    std::set<std::string> unitNames;

    std::string loader;

    UnitLuaWriter unitWriter;

    for (const UnitDefinition &unit : units) {
        if (unit.id < 0) {
            cleanupGeneratedFiles(generatedFiles);
            error = "Unit definition has a negative unit id";
            return false;
        }

        if (unit.name.empty()) {
            cleanupGeneratedFiles(generatedFiles);
            error = "Unit definition has an empty unit name";
            return false;
        }

        if (!unitIds.insert(unit.id).second) {
            cleanupGeneratedFiles(generatedFiles);
            error = "Duplicate unit id during Lua generation: " + std::to_string(unit.id);
            return false;
        }

        if (!unitNames.insert(unit.name).second) {
            cleanupGeneratedFiles(generatedFiles);
            error = "Duplicate unit name during Lua generation: " + unit.name;
            return false;
        }

        if (!unit.extractor) {
            continue;
        }

        const std::filesystem::path unitOutput = outputDirectory / (unit.name + ".lua");

        filesystemError.clear();

        if (std::filesystem::exists(unitOutput, filesystemError)) {
            cleanupGeneratedFiles(generatedFiles);

            if (filesystemError) {
                error = "Could not inspect unit Lua output '" + unitOutput.string() + "': " + filesystemError.message();
            } else {
                error = "Unit Lua output already exists: " + unitOutput.string();
            }

            return false;
        }

        UnitMetadata metadata;

        if (!resolver.resolve(
            static_cast<std::size_t>(unit.id),
            unit.name,
            metadata,
            error
        )) {
            cleanupGeneratedFiles(generatedFiles);
            return false;
        }

        if (!unitWriter.write(metadata, unitOutput, error)) {
            std::error_code cleanupError;
            std::filesystem::remove(unitOutput, cleanupError);
            cleanupGeneratedFiles(generatedFiles);
            return false;
        }

        generatedFiles.push_back(unitOutput);
        loader += "Load(\"luagen/units/" + unit.name + ".lua\")\n";
    }

    std::ofstream outputFile(output);

    if (!outputFile) {
        cleanupGeneratedFiles(generatedFiles);
        error = "Could not open units Lua loader for writing: " + output.string();
        return false;
    }

    outputFile << loader;

    if (!outputFile) {
        outputFile.close();
        std::error_code cleanupError;
        std::filesystem::remove(output, cleanupError);
        cleanupGeneratedFiles(generatedFiles);
        error = "Could not write units Lua loader: " + output.string();
        return false;
    }

    outputFile.close();
    return true;
}
