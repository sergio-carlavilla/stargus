// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef UNITS_DAT_DECODER_H
#define UNITS_DAT_DECODER_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct UnitDimensions
{
    std::uint16_t left = 0;
    std::uint16_t up = 0;
    std::uint16_t right = 0;
    std::uint16_t down = 0;
};

struct UnitsDatRecord
{
    std::uint8_t flingy = 0;

    std::uint32_t hitPoints = 0;
    std::uint8_t elevationLevel = 0;

    std::uint32_t specialAbilityFlags = 0;
    std::uint8_t sightRange = 0;

    std::uint16_t readySound = 0;
    std::uint16_t whatSoundStart = 0;
    std::uint16_t whatSoundEnd = 0;
    std::uint16_t pissSoundStart = 0;
    std::uint16_t pissSoundEnd = 0;
    std::uint16_t yesSoundStart = 0;
    std::uint16_t yesSoundEnd = 0;

    UnitDimensions dimensions;

    std::uint16_t portrait = 0;

    std::uint16_t mineralCost = 0;
    std::uint16_t vespeneCost = 0;
    std::uint16_t buildTime = 0;
};

class UnitsDatDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            std::vector<UnitsDatRecord> &records,
            std::string &error
        ) const;

};

#endif // UNITS_DAT_DECODER_H
