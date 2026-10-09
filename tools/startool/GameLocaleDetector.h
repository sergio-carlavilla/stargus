// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef GAMELOCALEDETECTOR_H
#define GAMELOCALEDETECTOR_H

#include "GameLocale.h"

#include <filesystem>
#include <string>
#include <vector>

struct InstalledGameLocale
{
    GameLocale locale;
    bool speech = false;
    bool text = false;

    bool isComplete() const
    {
        return speech && text;
    }
};

class GameLocaleDetector
{
    public:
        bool detect(
            const std::filesystem::path &installation,
            std::vector<InstalledGameLocale> &locales,
            std::string &errorMessage
        ) const;
};

#endif // GAMELOCALEDETECTOR_H
