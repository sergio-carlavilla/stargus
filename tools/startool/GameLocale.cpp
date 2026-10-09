// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "GameLocale.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
    std::string lowercase(std::string_view value)
    {
        std::string result(value);

        std::transform(
            result.begin(),
            result.end(),
            result.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(
                    std::tolower(character)
                );
            }
        );

        return result;
    }
}

std::string_view toString(GameLocale locale)
{
    switch (locale) {
        case GameLocale::EnUS:
            return "enUS";

        case GameLocale::FrFR:
            return "frFR";

        case GameLocale::ItIT:
            return "itIT";

        case GameLocale::DeDE:
            return "deDE";

        case GameLocale::EsES:
            return "esES";

        case GameLocale::EsMX:
            return "esMX";

        case GameLocale::PtBR:
            return "ptBR";

        case GameLocale::ZhCN:
            return "zhCN";

        case GameLocale::ZhTW:
            return "zhTW";

        case GameLocale::KoKR:
            return "koKR";

        case GameLocale::KoUS:
            return "koUS";

        case GameLocale::JaJP:
            return "jaJP";

        case GameLocale::PlPL:
            return "plPL";

        case GameLocale::RuRU:
            return "ruRU";
    }

    return "enUS";
}

std::string_view toCascLocaleNamespace(GameLocale locale)
{
    /*
     * StarCraft exposes two Korean localization profiles:
     *
     *   koKR - Korean transliterated localization
     *   koUS - Korean fully translated localization
     *
     * The CASC index does not expose a locales\koUS tree. The corresponding
     * resource namespace is exposed as locales\NOLA instead
     */
    if (locale == GameLocale::KoUS) {
        return "NOLA";
    }

    return toString(locale);
}

std::optional<GameLocale> parseGameLocale(
    std::string_view value
)
{
    const std::string locale = lowercase(value);

    if (locale == "enus") {
        return GameLocale::EnUS;
    }

    if (locale == "frfr") {
        return GameLocale::FrFR;
    }

    if (locale == "itit") {
        return GameLocale::ItIT;
    }

    if (locale == "dede") {
        return GameLocale::DeDE;
    }

    if (locale == "eses") {
        return GameLocale::EsES;
    }

    if (locale == "esmx") {
        return GameLocale::EsMX;
    }

    if (locale == "ptbr") {
        return GameLocale::PtBR;
    }

    if (locale == "zhcn") {
        return GameLocale::ZhCN;
    }

    if (locale == "zhtw") {
        return GameLocale::ZhTW;
    }

    if (locale == "kokr") {
        return GameLocale::KoKR;
    }

    if (locale == "kous") {
        return GameLocale::KoUS;
    }

    if (locale == "jajp") {
        return GameLocale::JaJP;
    }

    if (locale == "plpl") {
        return GameLocale::PlPL;
    }

    if (locale == "ruru") {
        return GameLocale::RuRU;
    }

    return std::nullopt;
}
