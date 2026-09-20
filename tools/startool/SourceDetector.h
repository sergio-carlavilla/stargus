// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef SOURCEDETECTOR_H
#define SOURCEDETECTOR_H

#include "GameSource.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class SourceDetector
{
    public:
        std::optional<GameSource> detect(
            const std::filesystem::path &path
        ) const;

    private:
        std::optional<std::filesystem::path> findFile(
            const std::filesystem::path &directory,
            const std::vector<std::string> &names
        ) const;

        static std::string lowercase(std::string value);
};

#endif // SOURCEDETECTOR_H
