// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef IMAGE_ASSET_RESOLVER_H
#define IMAGE_ASSET_RESOLVER_H

#include <cstddef>
#include <filesystem>
#include <string>

struct ImageAssetMetadata
{
    std::size_t index = 0;
    std::string grpName;
    std::string grpInput;
    std::string pngOutput;
    std::string luaId;
    std::string luaOutput;
    std::string palette;
    bool rgba = false;
    bool gfxTurns = false;
    bool save = true;
};

class ImageAssetResolver
{

    public:
        bool resolve(
            const std::filesystem::path &imagesDat,
            const std::filesystem::path &imagesTbl,
            std::size_t imageIndex,
            ImageAssetMetadata &metadata,
            std::string &error
        ) const;

};

#endif // IMAGE_ASSET_RESOLVER_H
