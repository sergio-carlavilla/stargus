// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ChkLuaWriter.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace
{

    constexpr int MinitileSubdivision = 4;

    constexpr std::array<const char *, 8> PlayerTypeNames = {
        "nobody",
        nullptr,
        nullptr,
        "rescue-passive",
        nullptr,
        "computer",
        "person",
        "neutral"
    };

    constexpr std::array<const char *, 5> RaceNames = {
        "zerg",
        "terran",
        "protoss",
        nullptr,
        "neutral"
    };

    bool makeUnitNames(
        const std::vector<UnitDefinition> &units,
        std::vector<std::string> &unitNames,
        std::string &error
    )
    {
        int maximumId = -1;

        for (const UnitDefinition &unit : units) {
            if (unit.id < 0) {
                error = "Unit definition has a negative id";
                return false;
            }

            if (unit.name.empty()) {
                error = "Unit definition has an empty name";
                return false;
            }

            if (unit.id > maximumId) {
                maximumId = unit.id;
            }
        }

        if (maximumId < 0) {
            error = "CHK Lua generation requires unit definitions";
            return false;
        }

        unitNames.assign(
            static_cast<std::size_t>(maximumId) + 1,
            std::string{}
        );

        for (const UnitDefinition &unit : units) {
            std::string &name = unitNames[
                static_cast<std::size_t>(unit.id)
            ];

            if (!name.empty()) {
                error =
                    "Duplicate unit id during CHK Lua generation: " +
                    std::to_string(unit.id);

                return false;
            }

            name = unit.name;
        }

        return true;
    }

    bool resolveLocation(
        const ChkMap &map,
        std::uint32_t index,
        const ChkLocation *&location,
        std::string &error
    )
    {
        if (index >= map.locations.size()) {
            error =
                "CHK trigger references invalid location index " +
                std::to_string(index);

            return false;
        }

        location = &map.locations[index];
        return true;
    }

    bool resolveString(
        const ChkMap &map,
        std::uint32_t oneBasedIndex,
        const std::string *&value,
        std::string &error
    )
    {
        static const std::string EmptyString;

        // CHK uses zero as "no string" for trigger actions.
        // Startool 3 indexed blindly with value - 1, which is unsafe for
        // malformed/edge-case maps such as zerg/09. Preserve the effective
        // output semantics by emitting an empty string instead of failing.
        if (oneBasedIndex == 0) {
            value = &EmptyString;
            return true;
        }

        const std::size_t index =
            static_cast<std::size_t>(oneBasedIndex - 1);

        if (index >= map.strings.size()) {
            error =
                "CHK trigger references invalid string index " +
                std::to_string(oneBasedIndex);

            return false;
        }

        value = &map.strings[index];
        return true;
    }

    std::uint16_t legacyCoordinate(
        std::uint32_t value
    )
    {
        return static_cast<std::uint16_t>(
            value * MinitileSubdivision
        );
    }

    bool writeTrigger(
        std::ostream &output,
        const ChkMap &map,
        const ChkTrigger &trigger,
        std::string &error
    )
    {
        for (const ChkTriggerCondition &condition : trigger.conditions) {
            if (condition.condition == 0) {
                break;
            }

            switch (condition.condition) {
                case 1:
                    output
                        << "-- CountdownTimer("
                        << condition.qualifiedNumber
                        << ")\n";
                    break;

                case 2:
                    output
                        << "-- Command("
                        << condition.group
                        << ", "
                        << condition.unitType
                        << ", "
                        << condition.qualifiedNumber
                        << ")\n";
                    break;

                case 3: {
                    const ChkLocation *location = nullptr;

                    if (!resolveLocation(
                        map,
                        condition.location,
                        location,
                        error
                    )) {
                        return false;
                    }

                    output
                        << "-- Bring("
                        << condition.group
                        << ", "
                        << condition.unitType
                        << ", ["
                        << legacyCoordinate(location->startX)
                        << ","
                        << legacyCoordinate(location->startY)
                        << "]-["
                        << legacyCoordinate(location->endX)
                        << ","
                        << legacyCoordinate(location->endY)
                        << "], "
                        << condition.qualifiedNumber
                        << ")\n";

                    break;
                }

                case 4:
                    output
                        << "-- Accumulate("
                        << condition.group
                        << ", "
                        << condition.qualifiedNumber
                        << ", "
                        << static_cast<unsigned int>(condition.resType)
                        << ")\n";
                    break;

                case 5:
                    output
                        << "-- Kill("
                        << condition.group
                        << ", "
                        << condition.unitType
                        << ", "
                        << condition.qualifiedNumber
                        << ")\n";
                    break;

                case 6:
                    output
                        << "-- CommandMost("
                        << condition.unitType
                        << ")\n";
                    break;

                case 7: {
                    const ChkLocation *location = nullptr;

                    if (!resolveLocation(
                        map,
                        condition.location,
                        location,
                        error
                    )) {
                        return false;
                    }

                    output
                        << "-- CommandMostAt("
                        << condition.unitType
                        << ", ["
                        << legacyCoordinate(location->startX)
                        << ","
                        << legacyCoordinate(location->startY)
                        << "]-["
                        << legacyCoordinate(location->endX)
                        << ","
                        << legacyCoordinate(location->endY)
                        << "])\n";

                    break;
                }

                case 8:
                    output
                        << "-- MostKills("
                        << condition.unitType
                        << ")\n";
                    break;

                case 9:
                    output
                        << "-- HighestScore("
                        << static_cast<unsigned int>(condition.resType)
                        << ")\n";
                    break;

                case 10:
                    output
                        << "-- MostResources("
                        << static_cast<unsigned int>(condition.resType)
                        << ")\n";
                    break;

                case 11:
                    output
                        << "-- Switch("
                        << static_cast<unsigned int>(condition.resType)
                        << ")\n";
                    break;

                case 12:
                    output
                        << "-- ElapsedTime("
                        << condition.qualifiedNumber
                        << ")\n";
                    break;

                case 13:
                    output << "-- MissionBriefing()\n";
                    break;

                case 14:
                    output
                        << "-- Opponents("
                        << condition.group
                        << ", "
                        << condition.qualifiedNumber
                        << ")\n";
                    break;

                case 15:
                    output
                        << "-- Deaths("
                        << condition.group
                        << ", "
                        << condition.unitType
                        << ", "
                        << condition.qualifiedNumber
                        << ")\n";
                    break;

                case 16:
                    output
                        << "-- CommandLeast("
                        << condition.unitType
                        << ")\n";
                    break;

                case 17: {
                    const ChkLocation *location = nullptr;

                    if (!resolveLocation(
                        map,
                        condition.location,
                        location,
                        error
                    )) {
                        return false;
                    }

                    output
                        << "-- CommandLeastAt("
                        << condition.unitType
                        << ", ["
                        << legacyCoordinate(location->startX)
                        << ","
                        << legacyCoordinate(location->startY)
                        << "]-["
                        << legacyCoordinate(location->endX)
                        << ","
                        << legacyCoordinate(location->endY)
                        << "])\n";

                    break;
                }

                case 18:
                    output
                        << "-- LeastKills("
                        << condition.unitType
                        << ")\n";
                    break;

                case 19:
                    output
                        << "-- LowestScore("
                        << static_cast<unsigned int>(condition.resType)
                        << ")\n";
                    break;

                case 20:
                    output
                        << "-- LeastResources("
                        << static_cast<unsigned int>(condition.resType)
                        << ")\n";
                    break;

                case 21:
                    output
                        << "-- Score("
                        << condition.group
                        << ", "
                        << static_cast<unsigned int>(condition.resType)
                        << ", "
                        << condition.qualifiedNumber
                        << ")\n";
                    break;

                case 22:
                    output << "-- Always()\n";
                    break;

                case 23:
                    output << "-- Never()\n";
                    break;

                default:
                    output
                        << "-- Unhandled condition: "
                        << static_cast<unsigned int>(condition.condition)
                        << "\n";
                    break;
            }
        }

        for (const ChkTriggerAction &action : trigger.actions) {
            if (action.action == 0) {
                break;
            }

            switch (action.action) {
                case 1:
                    output << "--  ActionVictory()\n";
                    break;

                case 2:
                    output << "--  ActionDefeat()\n";
                    break;

                case 3:
                    output << "--  Preserve trigger\n";
                    break;

                case 4:
                    output
                        << "--  Wait("
                        << action.time
                        << ")\n";
                    break;

                case 5:
                    output << "--  Pause\n";
                    break;

                case 6:
                    output << "--  Unpause\n";
                    break;

                case 7: {
                    const std::string *value = nullptr;
                    const ChkLocation *location = nullptr;

                    if (!resolveString(
                        map,
                        action.triggerNumber,
                        value,
                        error
                    )) {
                        return false;
                    }

                    if (!resolveLocation(
                        map,
                        action.source,
                        location,
                        error
                    )) {
                        return false;
                    }

                    output
                        << "--  Transmission("
                        << *value
                        << ", "
                        << action.status
                        << ", ["
                        << legacyCoordinate(location->startX)
                        << ","
                        << legacyCoordinate(location->startY)
                        << "]-["
                        << legacyCoordinate(location->endX)
                        << ","
                        << legacyCoordinate(location->endY)
                        << "], "
                        << action.time
                        << ", "
                        << static_cast<unsigned int>(action.numUnits)
                        << ", "
                        << action.wavNumber
                        << ", "
                        << action.time
                        << ")\n";

                    break;
                }

                case 8:
                    output
                        << "--  PlayWav("
                        << action.wavNumber
                        << ", "
                        << action.time
                        << ")\n";
                    break;

                case 9: {
                    const std::string *value = nullptr;

                    if (!resolveString(
                        map,
                        action.triggerNumber,
                        value,
                        error
                    )) {
                        return false;
                    }

                    output
                        << "--  TextMessage("
                        << *value
                        << ")\n";

                    break;
                }

                case 10: {
                    const ChkLocation *location = nullptr;

                    if (!resolveLocation(
                        map,
                        action.source,
                        location,
                        error
                    )) {
                        return false;
                    }

                    const std::uint16_t x =
                        static_cast<std::uint16_t>(
                            (
                                location->startX * MinitileSubdivision +
                                location->endX * MinitileSubdivision
                            ) /
                            2 /
                            32
                        );

                    const std::uint16_t y =
                        static_cast<std::uint16_t>(
                            (
                                location->startY * MinitileSubdivision +
                                location->endY * MinitileSubdivision
                            ) /
                            2 /
                            32
                        );

                    output
                        << "--  CenterMap("
                        << x
                        << ", "
                        << y
                        << ")\n";

                    break;
                }

                case 12: {
                    const std::string *value = nullptr;

                    if (!resolveString(
                        map,
                        action.triggerNumber,
                        value,
                        error
                    )) {
                        return false;
                    }

                    output
                        << "--  SetObjectives("
                        << *value
                        << ")\n";

                    break;
                }

                case 26:
                    output
                        << "--  SetResources("
                        << action.firstGroup
                        << ", "
                        << action.secondGroup
                        << ", "
                        << static_cast<unsigned int>(action.numUnits)
                        << ", "
                        << action.status
                        << ")\n";
                    break;

                case 30:
                    output << "--  Mute unit speech\n";
                    break;

                case 31:
                    output << "--  Unmute unit speech\n";
                    break;

                default:
                    output
                        << "--  Unhandled action: "
                        << static_cast<unsigned int>(action.action)
                        << "\n";
                    break;
            }
        }

        return true;
    }

    bool renderSmp(
        const ChkMap &map,
        std::string &content,
        std::string &error
    )
    {
        std::ostringstream output;

        output << "-- Stratagus Map Presentation\n";
        output << "-- File generated automatically by Stargus\n";
        output << "\n";

        output << "DefinePlayerTypes(";

        bool first = true;

        for (
            std::size_t player = 0;
            player < ChkPlayerCount;
            ++player
        ) {
            const int type = map.playerType[player];

            if (
                type < 0 ||
                static_cast<std::size_t>(type) >=
                    PlayerTypeNames.size()
            ) {
                error =
                    "CHK contains invalid player type " +
                    std::to_string(type);

                return false;
            }

            const char *name =
                PlayerTypeNames[
                    static_cast<std::size_t>(type)
                ];

            if (name == nullptr) {
                continue;
            }

            if (!first) {
                output << ",";
            }

            first = false;

            output
                << "\""
                << name
                << "\"";
        }

        output << ")\n";

        output
            << "PresentMap(\""
            << map.description
            << "\", 2, "
            << map.width * MinitileSubdivision
            << ", "
            << map.height * MinitileSubdivision
            << ", 0)\n";

        content = output.str();
        return true;
    }

    bool renderSms(
        const ChkMap &map,
        const std::vector<std::string> &unitNames,
        std::string &content,
        std::string &error
    )
    {
        std::ostringstream output;

        output << "-- Stratagus Map Setup\n";
        output << "-- File generated automatically by Stargus\n";
        output << "\n";

        for (
            std::size_t player = 0;
            player < ChkPlayerCount;
            ++player
        ) {
            if (map.playerType[player] == 0) {
                continue;
            }

            const int race = map.playerRace[player];

            if (
                race < 0 ||
                static_cast<std::size_t>(race) >= RaceNames.size() ||
                RaceNames[static_cast<std::size_t>(race)] == nullptr
            ) {
                error =
                    "CHK contains unsupported race " +
                    std::to_string(race) +
                    " for active player " +
                    std::to_string(player);

                return false;
            }

            output
                << "SetStartView("
                << player
                << ", "
                << map.playerStart[player].x * MinitileSubdivision
                << ", "
                << map.playerStart[player].y * MinitileSubdivision
                << ")\n";

            output
                << "SetPlayerData("
                << player
                << ", \"Resources\", \"minerals\", 0)\n";

            output
                << "SetPlayerData("
                << player
                << ", \"Resources\", \"gas\", 0)\n";

            output
                << "SetPlayerData("
                << player
                << ", \"RaceName\", \""
                << RaceNames[static_cast<std::size_t>(race)]
                << "\")\n";
        }

        output << "\n\n";
        output << "LoadTileModels(\"luagen/tilesets/" << map.terrain << ".lua\")\n";
        output << "\n\n";

        if (map.width <= 0 || map.height <= 0) {
            error = "CHK map dimensions are invalid";
            return false;
        }

        const std::size_t expectedTiles = static_cast<std::size_t>(map.width) * static_cast<std::size_t>(map.height);
        if (map.tiles.size() != expectedTiles) {
            error = "CHK tile count does not match map dimensions";
            return false;
        }

        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                const std::size_t tileIndex = static_cast<std::size_t>(y) * static_cast<std::size_t>(map.width) + static_cast<std::size_t>(x);

                output
                    << "SetTile("
                    << map.tiles[tileIndex]
                    << ", "
                    << x * MinitileSubdivision
                    << ", "
                    << y * MinitileSubdivision
                    << ")\n";
            }
        }

        output << "\n\n";

        for (const ChkUnit &unit : map.units) {
            const std::size_t unitType = static_cast<std::size_t>(unit.type);

            if (unitType >= unitNames.size() || unitNames[unitType].empty()) {
                error = "CHK references unknown unit type " + std::to_string(unit.type);
                return false;
            }

            output
                << "unit = CreateUnit(\""
                << unitNames[unitType]
                << "\", "
                << static_cast<unsigned int>(unit.player)
                << ", {"
                << unit.x * MinitileSubdivision
                << ", "
                << unit.y * MinitileSubdivision
                << "})\n";

            if (unit.resourceAmount != 0) {
                output << "SetResourcesHeld(unit, " << unit.resourceAmount << ")\n";
            }
        }

        output << "\n\n";

        for (const ChkTrigger &trigger : map.triggers) {
            if (!writeTrigger(output, map, trigger, error)) {
                return false;
            }
        }

        output << "\n\n";
        output << "if(preferences.RapidStratagusIDE == true) then\n";
        output << "Load(\"RapidStratagusIDE/RSI_Functions.lua\")\n";
        output << "RSI_MapConfiguration()\n";
        output << "end\n";
        output << "\n";
        output << "\n\n";

        content = output.str();
        return true;
    }

    bool writeFile(
        const std::filesystem::path &path,
        const std::string &content,
        std::string &error
    )
    {
        std::error_code filesystemError;

        if (std::filesystem::exists(path, filesystemError)) {
            if (filesystemError) {
                error = "Could not inspect Lua output '" + path.string() + "': " + filesystemError.message();
                return false;
            }

            error = "Lua output already exists: " + path.string();
            return false;
        }

        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories( parent, filesystemError);

            if (filesystemError) {
                error = "Could not create Lua output directory: " + filesystemError.message();
                return false;
            }
        }

        std::ofstream output(path, std::ios::binary);
        if (!output) {
            error = "Could not open Lua output for writing: " + path.string();
            return false;
        }

        output.write(content.data(), static_cast<std::streamsize>(content.size()));

        if (!output) {
            output.close();

            std::error_code cleanupError;
            std::filesystem::remove(path, cleanupError);

            error = "Could not write Lua output: " + path.string();
            return false;
        }

        output.close();
        return true;
    }

}

bool ChkLuaWriter::write(
    const ChkMap &map,
    const std::vector<UnitDefinition> &units,
    const std::filesystem::path &outputBase,
    std::string &error
) const
{
    std::vector<std::string> unitNames;

    if (!makeUnitNames(units, unitNames, error)) {
        return false;
    }

    std::string smpContent;
    std::string smsContent;

    if (!renderSmp(map, smpContent, error)) {
        return false;
    }

    if (!renderSms(map, unitNames, smsContent, error)) {
        return false;
    }

    const std::filesystem::path smpOutput(outputBase.string() + "smp");
    const std::filesystem::path smsOutput(outputBase.string() + "sms");

    if (!writeFile( smpOutput, smpContent, error)) {
        return false;
    }

    if (!writeFile(smsOutput, smsContent, error)) {
        std::error_code cleanupError;
        std::filesystem::remove(smpOutput, cleanupError);
        return false;
    }

    return true;
}
