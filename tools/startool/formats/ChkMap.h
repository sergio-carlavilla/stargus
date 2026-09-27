// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef CHK_MAP_H
#define CHK_MAP_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr std::size_t ChkPlayerCount = 16;
constexpr std::uint16_t ChkStartLocationUnit = 214;

struct ChkPlayerStart
{
    int x = 0;
    int y = 0;
};

struct ChkUnit
{
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t type = 0;
    std::uint16_t properties = 0;
    std::uint16_t validElements = 0;
    std::uint8_t player = 0;
    std::uint8_t hitPointsPercent = 0;
    std::uint8_t shieldPointsPercent = 0;
    std::uint8_t energyPointsPercent = 0;
    std::uint32_t resourceAmount = 0;
    std::uint16_t numUnitsIn = 0;
    std::uint8_t stateFlags = 0;
};

struct ChkLocation
{
    std::uint32_t startX = 0;
    std::uint32_t startY = 0;
    std::uint32_t endX = 0;
    std::uint32_t endY = 0;
    std::uint16_t stringNumber = 0;
    std::uint16_t flags = 0;
};

struct ChkTriggerCondition
{
    std::uint32_t location = 0;
    std::uint32_t group = 0;
    std::uint32_t qualifiedNumber = 0;
    std::uint16_t unitType = 0;
    std::uint8_t compType = 0;
    std::uint8_t condition = 0;
    std::uint8_t resType = 0;
    std::uint8_t flags = 0;
};

struct ChkTriggerAction
{
    std::uint32_t source = 0;
    std::uint32_t triggerNumber = 0;
    std::uint32_t wavNumber = 0;
    std::uint32_t time = 0;
    std::uint32_t firstGroup = 0;
    std::uint32_t secondGroup = 0;
    std::uint16_t status = 0;
    std::uint8_t action = 0;
    std::uint8_t numUnits = 0;
    std::uint8_t actionFlags = 0;
};

struct ChkTrigger
{
    std::array<ChkTriggerCondition, 16> conditions;
    std::array<ChkTriggerAction, 64> actions;
};

struct ChkMap
{
    int width = 0;
    int height = 0;

    std::string terrain;
    std::string description = "none";

    std::array<int, ChkPlayerCount> playerRace{};
    std::array<int, ChkPlayerCount> playerType{};
    std::array<ChkPlayerStart, ChkPlayerCount> playerStart{};

    std::vector<std::uint16_t> tiles;
    std::vector<ChkLocation> locations;
    std::vector<ChkTrigger> triggers;
    std::vector<ChkUnit> units;
    std::vector<std::string> strings;
};

#endif // CHK_MAP_H
