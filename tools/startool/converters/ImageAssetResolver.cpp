// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ImageAssetResolver.h"

#include "formats/ImagesDatDecoder.h"
#include "formats/TblDecoder.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace
{

    constexpr std::uint8_t DrawFunctionRemapping = 9;
    constexpr std::uint8_t DrawFunctionShadow = 10;

    std::string toLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            }
        );

        return value;
    }

    void replaceAll(std::string &value, char from, char to)
    {
        std::replace(value.begin(), value.end(), from, to);
    }

    std::string stripGrpExtension(std::string value)
    {
        constexpr const char *extension = ".grp";

        if (
            value.size() >= 4 &&
            value.compare(value.size() - 4, 4, extension) == 0
        ) {
            value.resize(value.size() - 4);
        }

        return value;
    }

    std::vector<std::string> splitPath(const std::string &value)
    {
        std::vector<std::string> components;
        std::size_t start = 0;

        while (start <= value.size()) {
            const std::size_t slash = value.find('/', start);

            if (slash == std::string::npos) {
                components.push_back(value.substr(start));
                break;
            }

            components.push_back(value.substr(start, slash - start));
            start = slash + 1;
        }

        return components;
    }

    std::string remappingPalette(std::uint8_t remapping)
    {
        switch (remapping) {
            case 1:
                return "ofire";
            case 2:
                return "gfire";
            case 3:
                return "bfire";
            case 4:
                return "bexpl";
            default:
                // Preserve Startool legacy's fallback for unknown remapping values
                return "ofire";
        }
    }

}

bool ImageAssetResolver::resolve(
    const std::filesystem::path &imagesDat,
    const std::filesystem::path &imagesTbl,
    std::size_t imageIndex,
    ImageAssetMetadata &metadata,
    std::string &error
) const
{
    ImagesDatDecoder imagesDecoder;
    std::vector<ImagesDatRecord> images;

    if (!imagesDecoder.decode(imagesDat, images, error)) {
        return false;
    }

    if (imageIndex >= images.size()) {
        error = "Image index " + std::to_string(imageIndex) + " exceeds images.dat entry count " + std::to_string(images.size());
        return false;
    }

    TblDecoder tblDecoder;
    std::vector<TblEntry> imageNames;

    if (!tblDecoder.decode(imagesTbl, imageNames, error)) {
        return false;
    }

    const ImagesDatRecord &record = images[imageIndex];

    if (record.grp == 0) {
        error = "Image " + std::to_string(imageIndex) + " has a null images.tbl reference";
        return false;
    }

    const std::size_t tblIndex = static_cast<std::size_t>(record.grp - 1);

    if (tblIndex >= imageNames.size()) {
        error = "Image " + std::to_string(imageIndex) + " references images.tbl entry " + std::to_string(tblIndex) + " but the table contains only " + std::to_string(imageNames.size()) + " entries";
        return false;
    }

    std::string grpName = toLower(imageNames[tblIndex].name);

    if (grpName.empty()) {
        error = "Image " + std::to_string(imageIndex) + " resolves to an empty GRP name";
        return false;
    }

    replaceAll(grpName, '\\', '/');

    metadata = ImageAssetMetadata{};
    metadata.index = imageIndex;
    metadata.grpName = grpName;
    metadata.grpInput = "unit/" + grpName;
    metadata.gfxTurns = record.gfxTurns;
    metadata.palette = "tunit";

    std::string remapping;

    if (record.drawFunction == DrawFunctionRemapping) {
        remapping = remappingPalette(record.remapping);
        metadata.palette = remapping;
        metadata.rgba = true;
    } else if (record.drawFunction == DrawFunctionShadow) {
        metadata.save = false;
    } else {
        const std::vector<std::string> components = splitPath(grpName);

        if (
            components.size() >= 4 &&
            components[0] == "thingy" &&
            components[1] == "tileset" &&
            !components[2].empty()
        ) {
            metadata.palette = components[2];
        }
    }

    // Preserve Startool legacy's explicit exceptions.
    if (grpName == "thingy/blackx.grp") {
        metadata.save = false;
    } else if (grpName == "terran/tank.grp") {
        metadata.palette = "badlands";
    } else if (grpName == "neutral/cbattle.grp") {
        metadata.palette = "badlands";
    } else if (grpName == "neutral/ion.grp") {
        metadata.palette = "platform";
    } else if (grpName == "neutral/khyad01.grp") {
        metadata.palette = "jungle";
    } else if (grpName == "neutral/temple.grp") {
        metadata.palette = "jungle";
    } else if (grpName == "neutral/geyser.grp") {
        metadata.palette = "badlands";
    }

    std::string storageBase = "unit/" + stripGrpExtension(grpName);

    if (!remapping.empty()) {
        storageBase += "_" + remapping;
    }

    metadata.pngOutput = "graphics/" + storageBase + ".png";

    std::string imageName = stripGrpExtension(grpName);
    replaceAll(imageName, '/', '_');

    metadata.luaId = "image_" + std::to_string(imageIndex) + "_" + imageName;
    metadata.luaOutput = "luagen/images/" + metadata.luaId + ".lua";

    return true;
}
