// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PORTRAIT_ASSET_RESOLVER_H
#define PORTRAIT_ASSET_RESOLVER_H

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct PortraitAssetMetadata
{
    std::size_t index = 0;
    std::string id;
    std::string idleInputBase;
    std::string talkingInputBase;
    std::string idleOutputBase;
    std::string talkingOutputBase;
};

class PortraitAssetResolver
{
    public:
        bool resolve(
            const std::filesystem::path &portdataDat,
            const std::filesystem::path &portdataTbl,
            std::vector<PortraitAssetMetadata> &portraits,
            std::string &error
        ) const;
};

#endif // PORTRAIT_ASSET_RESOLVER_H
