// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PALETTE_DEFINITION_H
#define PALETTE_DEFINITION_H

#include <optional>
#include <string>

enum class PaletteDefinitionKind
{
    Pcx,
    Pcx2D,
    Wpe
};

struct PaletteMapping
{
    int length = 0;
    int start = 0;
    int index = 0;
};

struct PaletteDefinition
{
    std::string id;

    PaletteDefinitionKind kind = PaletteDefinitionKind::Pcx;

    std::string source;
    std::string input;

    std::optional<PaletteMapping> mapping;
};

#endif // PALETTE_DEFINITION_H
