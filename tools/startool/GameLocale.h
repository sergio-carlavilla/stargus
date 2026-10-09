// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef GAMELOCALE_H
#define GAMELOCALE_H

#include <optional>
#include <string_view>

enum class GameLocale
{
    EnUS,
    FrFR,
    ItIT,
    DeDE,
    EsES,
    EsMX,
    PtBR,
    ZhCN,
    ZhTW,
    KoKR,
    KoUS,
    JaJP,
    PlPL,
    RuRU
};

std::string_view toString(GameLocale locale);

std::string_view toCascLocaleNamespace(GameLocale locale);

std::optional<GameLocale> parseGameLocale(
    std::string_view value
);

#endif // GAMELOCALE_H
