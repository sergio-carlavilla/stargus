// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "UnitLuaWriter.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

    std::string boolean(bool value)
    {
        return value ? "true" : "false";
    }

    std::string escapeLuaString(const std::string &value)
    {
        std::string escaped;
        escaped.reserve(value.size());

        for (const char character : value) {
            switch (character) {
                case '\\':
                    escaped += "\\\\";
                    break;

                case '"':
                    escaped += "\\\"";
                    break;

                case '\n':
                    escaped += "\\n";
                    break;

                case '\r':
                    escaped += "\\r";
                    break;

                case '\t':
                    escaped += "\\t";
                    break;

                default:
                    escaped += character;
                    break;
            }
        }

        return escaped;
    }

    std::string quote(const std::string &value)
    {
        return "\"" + escapeLuaString(value) + "\"";
    }

    std::string renderQuotedList(const std::vector<std::string> &values)
    {
        std::ostringstream stream;

        stream << "{";

        for (std::size_t index = 0; index < values.size(); ++index) {
            if (index != 0) {
                stream << ", ";
            }

            stream << quote(values[index]);
        }

        stream << "}";

        return stream.str();
    }

    std::string renderSounds(const UnitMetadata &metadata)
    {
        std::vector<std::string> values;

        if (!metadata.readySounds.empty()) {
            values.push_back("ready");
            values.push_back(metadata.ident + "-sound-ready");
        }

        if (!metadata.yesSounds.empty()) {
            values.push_back("acknowledge");
            values.push_back(metadata.ident + "-sound-yes");
        }

        std::vector<std::string> selected;

        if (!metadata.whatSounds.empty()) {
            selected.push_back(metadata.ident + "-sound-what");
        }

        if (!metadata.pissSounds.empty()) {
            selected.push_back(metadata.ident + "-sound-piss");
        }

        if (selected.size() > 1) {
            values.push_back("selected");
            values.push_back(metadata.ident + "-sound-selected");
        } else if (selected.size() == 1) {
            values.push_back("selected");
            values.push_back(selected.front());
        }

        return renderQuotedList(values);
    }

    void writeMakeSound(
        std::ostringstream &stream,
        const std::string &id,
        const std::vector<std::string> &sounds
    )
    {
        if (sounds.empty()) {
            return;
        }

        stream << "MakeSound(" << quote(id) << ", " << renderQuotedList(sounds) << ")\n";
    }

}

std::string UnitLuaWriter::render(const UnitMetadata &metadata) const
{
    std::ostringstream stream;
    const std::string type = metadata.airUnit ? "fly" : "land";

    stream << "DefineUnitType(" << quote(metadata.ident) << ", {Name = " << quote(metadata.displayName) << "\n";
    stream << ", Image = " << metadata.imageId << "\n";
    stream << ", Shadow = {\"offset\", {" << metadata.shadowX << ", " << metadata.shadowY << "}, \"scale\", 1}\n";
    stream << ", Icon = \"icon-terran-command-center\"\n";
    stream << ", Animations = \"animations-dummy-still\"\n";
    stream << ", Portrait = " << metadata.portraitId << "\n";
    stream << ", HitPoints = " << metadata.hitPoints << "\n";
    stream << ", TileSize = {" << metadata.tileWidth << ", " << metadata.tileHeight << "}\n";
    stream << ", BoxSize = {" << metadata.boxWidth << ", " << metadata.boxHeight << "}\n";
    stream << ", SightRange = " << metadata.sightRange << "\n";
    stream << ", ComputerReactionRange = math.ceil(" << metadata.sightRange << " * ComputerReactionRangeFactor)\n";
    stream << ", PersonReactionRange = math.floor(" << metadata.sightRange << " * PersonReactionRangeFactor)\n";
    stream << ", NumDirections = " << metadata.imageId << "_NumDirections\n";
    stream << ", AirUnit = " << boolean(metadata.airUnit) << "\n";
    stream << ", Type = " << quote(type) << "\n";
    stream << ", Building = " << boolean(metadata.building) << "\n";
    stream << ", organic = " << boolean(metadata.organic) << "\n";
    stream << ", LandUnit = " << boolean(metadata.landUnit) << "\n";
    stream << ", Costs = {\"time\", " << metadata.buildTime << ", \"minerals\", " << metadata.mineralCost << ", \"gas\", " << metadata.gasCost << "}\n";
    stream << ", PersonalSpace = {1, 1}\n";
    stream << ", Sounds = " << renderSounds(metadata) << "\n";
    stream << "})\n";

    writeMakeSound(stream, metadata.ident + "-sound-ready", metadata.readySounds);
    writeMakeSound(stream, metadata.ident + "-sound-what", metadata.whatSounds);
    writeMakeSound(stream, metadata.ident + "-sound-yes", metadata.yesSounds);
    writeMakeSound(stream, metadata.ident + "-sound-piss", metadata.pissSounds);

    if (!metadata.whatSounds.empty() && !metadata.pissSounds.empty()) {
        stream
            << "MakeSoundGroup("
            << quote(metadata.ident + "-sound-selected")
            << ", "
            << quote(metadata.ident + "-sound-what")
            << ", "
            << quote(metadata.ident + "-sound-piss")
            << ")\n";
    }

    return stream.str();
}

bool UnitLuaWriter::write(
    const UnitMetadata &metadata,
    const std::filesystem::path &output,
    std::string &error
) const
{
    std::error_code filesystemError;

    if (!output.parent_path().empty()) {
        std::filesystem::create_directories(
            output.parent_path(),
            filesystemError
        );

        if (filesystemError) {
            error = "Could not create unit Lua directory: " + filesystemError.message();
            return false;
        }
    }

    std::ofstream file(output);

    if (!file) {
        error = "Could not open unit Lua output: " + output.string();
        return false;
    }

    file << render(metadata);
    if (!file) {
        error = "Could not write unit Lua output: " + output.string();
        return false;
    }

    return true;
}
