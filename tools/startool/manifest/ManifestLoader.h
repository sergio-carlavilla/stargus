// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MANIFEST_LOADER_H
#define MANIFEST_LOADER_H

#include "Manifest.h"

#include <filesystem>
#include <optional>
#include <string>

class ManifestLoader
{
    public:
        std::optional<Manifest> load(
            const std::filesystem::path &path,
            std::string &error
        ) const;
};

#endif // MANIFEST_LOADER_H
