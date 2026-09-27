// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef UNIT_METADATA_H
#define UNIT_METADATA_H

#include <cstddef>
#include <string>
#include <vector>

struct UnitMetadata
{
    std::size_t unitId = 0;

    std::string ident;
    std::string displayName;

    std::size_t flingyId = 0;
    std::size_t spriteId = 0;
    std::size_t imageIndex = 0;

    std::string imageId;
    std::string portraitId;

    int hitPoints = 0;

    int tileWidth = 0;
    int tileHeight = 0;

    int boxWidth = 0;
    int boxHeight = 0;

    int sightRange = 0;

    int shadowX = -7;
    int shadowY = -7;

    bool airUnit = false;
    bool building = false;
    bool organic = false;
    bool landUnit = true;

    int buildTime = 0;
    int mineralCost = 0;
    int gasCost = 0;

    std::vector<std::string> readySounds;
    std::vector<std::string> whatSounds;
    std::vector<std::string> yesSounds;
    std::vector<std::string> pissSounds;
};

#endif // UNIT_METADATA_H
