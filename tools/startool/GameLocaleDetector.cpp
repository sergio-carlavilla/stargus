// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "GameLocaleDetector.h"

#include <array>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    const std::array<GameLocale, 14> supportedLocales = {
        GameLocale::EnUS,
        GameLocale::FrFR,
        GameLocale::ItIT,
        GameLocale::DeDE,
        GameLocale::EsES,
        GameLocale::EsMX,
        GameLocale::PtBR,
        GameLocale::ZhCN,
        GameLocale::ZhTW,
        GameLocale::KoKR,
        GameLocale::KoUS,
        GameLocale::JaJP,
        GameLocale::PlPL,
        GameLocale::RuRU
    };

    std::vector<std::string> split(
        const std::string &value,
        char separator
    )
    {
        std::vector<std::string> parts;

        std::size_t begin = 0;

        while (true) {
            const std::size_t end = value.find(separator, begin);
            if (end == std::string::npos) {
                parts.emplace_back(value.substr(begin));
                break;
            }

            parts.emplace_back(value.substr(begin, end - begin));
            begin = end + 1;
        }

        return parts;
    }

    bool containsLocaleTag(
        const std::string &tags,
        GameLocale locale,
        std::string_view component
    )
    {
        const std::string localeName(toString(locale));
        const std::string tag = localeName + " " + std::string(component) + "?";
        if (tags.find(tag) != std::string::npos) {
            return true;
        }

        /*
         * koUS is the public StarCraft locale identifier for the second
         * Korean localization profile. Some internal Battle.net/CASC data
         * uses NOLA for that profile
         */
        if (locale == GameLocale::KoUS) {
            const std::string internalTag = "NOLA " + std::string(component) + "?";
            return tags.find(internalTag) != std::string::npos;
        }

        return false;
    }
}

bool GameLocaleDetector::detect(
    const std::filesystem::path &installation,
    std::vector<InstalledGameLocale> &locales,
    std::string &errorMessage
) const
{
    locales.clear();
    errorMessage.clear();

    const std::filesystem::path buildInfo = installation / ".build.info";
    std::ifstream input(buildInfo);
    if (!input) {
        errorMessage = "Could not open Battle.net build metadata: " + buildInfo.string();
        return false;
    }

    std::string headerLine;
    std::string dataLine;

    if (!std::getline(input, headerLine) || !std::getline(input, dataLine)) {
        errorMessage = "Invalid Battle.net build metadata: " + buildInfo.string();
        return false;
    }

    const std::vector<std::string> headers = split(headerLine, '|');
    const std::vector<std::string> values = split(dataLine, '|');
    if (headers.size() != values.size()) {
        errorMessage = "Malformed Battle.net build metadata: " + buildInfo.string();
        return false;
    }

    std::string tags;

    for (std::size_t index = 0; index < headers.size(); ++index) {
        if (headers[index].rfind("Tags!", 0) == 0) {
            tags = values[index];
            break;
        }
    }

    if (tags.empty()) {
        errorMessage = "Battle.net build metadata does not contain Tags";
        return false;
    }

    for (const GameLocale locale : supportedLocales) {
        InstalledGameLocale installedLocale{
            locale,
            containsLocaleTag(tags, locale, "speech"),
            containsLocaleTag(tags, locale, "text")
        };

        if (installedLocale.speech || installedLocale.text) {
            locales.push_back(installedLocale);
        }
    }

    return true;
}
