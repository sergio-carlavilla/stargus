// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef LOADED_PALETTE_H
#define LOADED_PALETTE_H

#include "Palette.h"
#include "Palette2D.h"

#include <variant>

using LoadedPalette = std::variant<Palette, Palette2D>;

#endif // LOADED_PALETTE_H
