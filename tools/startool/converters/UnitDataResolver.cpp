// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "UnitDataResolver.h"

#include "formats/FlingyDatDecoder.h"
#include "formats/ImagesDatDecoder.h"
#include "formats/PortDataDecoder.h"
#include "formats/SfxDataDecoder.h"
#include "formats/SpritesDatDecoder.h"
#include "formats/TblDecoder.h"
#include "formats/UnitsDatDecoder.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <vector>

namespace
{

    constexpr std::uint16_t PortraitNone = 65535;
    constexpr std::uint16_t SoundNone = 0;

    constexpr std::uint32_t BuildingFlag = 0x00000001U;
    constexpr std::uint32_t FlyerFlag = 0x00000004U;
    constexpr std::uint32_t OrganicFlag = 0x00010000U;

    std::string toLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(
                    std::tolower(character)
                );
            }
        );

        return value;
    }

    std::string makeImageIdentifier(
        std::size_t imageId,
        std::string name
    )
    {
        std::replace(name.begin(), name.end(), '\\', '_');

        std::replace(name.begin(), name.end(), '/', '_');

        name = toLower(std::move(name));

        const std::size_t extension = name.find_last_of('.');
        if (extension != std::string::npos) {
            name.erase(extension);
        }

        return "image_" + std::to_string(imageId) + "_" + name;
    }

    std::string makePortraitIdentifier(std::string portrait)
    {
        std::replace( portrait.begin(), portrait.end(), '\\', '/');

        const std::filesystem::path path(portrait);

        return "portrait_" + toLower(path.parent_path().generic_string());
    }

    bool resolveSound(
        const std::vector<SfxDataRecord> &sfxData,
        const std::vector<TblEntry> &sfxTable,
        std::size_t soundId,
        std::string &sound,
        std::string &error
    )
    {
        if (soundId >= sfxData.size()) {
            error = "Sound index " + std::to_string(soundId) + " is out of range";
            return false;
        }

        const std::uint32_t tableReference = sfxData[soundId].soundFile;

        if (tableReference == 0) {
            error = "Sound " + std::to_string(soundId) + " has a null sfxdata.tbl reference";
            return false;
        }

        const std::size_t tableIndex = static_cast<std::size_t>(tableReference - 1);

        if (tableIndex >= sfxTable.size()) {
            error = "Sound " + std::to_string(soundId) + " references invalid sfxdata.tbl entry " + std::to_string(tableIndex);
            return false;
        }

        std::string path = toLower(sfxTable[tableIndex].name);

        std::replace(path.begin(), path.end(), '\\', '/');

        if (path.size() >= 4 && path.compare( path.size() - 4, 4, ".wav") == 0) {
            path.erase(path.size() - 4);
        }

        sound = "sounds/unit/" + path + ".ogg";

        return true;
    }

    bool resolveSoundRange(
        const std::vector<SfxDataRecord> &sfxData,
        const std::vector<TblEntry> &sfxTable,
        std::uint16_t start,
        std::uint16_t end,
        std::vector<std::string> &sounds,
        std::string &error
    )
    {
        sounds.clear();

        // Preserve Startool legacy behaviour:
        // (start || end) == sound_none
        if (start == SoundNone && end == SoundNone) {
            return true;
        }

        for (std::size_t id = start; id <= end; ++id) {
            std::string sound;

            if (!resolveSound(sfxData, sfxTable, id, sound, error)) {
                return false;
            }

            sounds.push_back(std::move(sound));
        }

        return true;
    }

}

bool UnitDataResolver::resolve(
    const UnitDataFiles &files,
    std::size_t unitId,
    const std::string &ident,
    UnitMetadata &metadata,
    std::string &error
) const
{
    UnitsDatDecoder unitsDecoder;
    FlingyDatDecoder flingyDecoder;
    SpritesDatDecoder spritesDecoder;
    ImagesDatDecoder imagesDecoder;
    PortDataDecoder portdataDecoder;
    SfxDataDecoder sfxdataDecoder;
    TblDecoder tblDecoder;

    std::vector<UnitsDatRecord> units;
    std::vector<FlingyDatRecord> flingies;
    std::vector<SpritesDatRecord> sprites;
    std::vector<ImagesDatRecord> images;
    std::vector<PortDataRecord> portraits;
    std::vector<SfxDataRecord> sfxData;

    std::vector<TblEntry> imageNames;
    std::vector<TblEntry> portraitNames;
    std::vector<TblEntry> soundNames;
    std::vector<TblEntry> unitNames;

    if (!unitsDecoder.decode(files.unitsDat, units, error)) {
        return false;
    }

    if (!flingyDecoder.decode(files.flingyDat, flingies, error)) {
        return false;
    }

    if (!spritesDecoder.decode(files.spritesDat, sprites, error)) {
        return false;
    }

    if (!imagesDecoder.decode(files.imagesDat, images, error)) {
        return false;
    }

    if (!portdataDecoder.decode(files.portdataDat, portraits, error)) {
        return false;
    }

    if (!sfxdataDecoder.decode(files.sfxdataDat, sfxData, error)) {
        return false;
    }

    if (!tblDecoder.decode(files.imagesTbl, imageNames, error)) {
        return false;
    }

    if (!tblDecoder.decode(files.portdataTbl, portraitNames, error)) {
        return false;
    }

    if (!tblDecoder.decode(files.sfxdataTbl, soundNames, error)) {
        return false;
    }

    if (!tblDecoder.decode(files.statTxtTbl, unitNames, error)) {
        return false;
    }

    if (unitId >= units.size()) {
        error = "Unit index " + std::to_string(unitId) + " is out of range";
        return false;
    }

    if (unitId >= unitNames.size()) {
        error = "Unit index " + std::to_string(unitId) + " is missing from stat_txt.tbl";
        return false;
    }

    const UnitsDatRecord &unit = units[unitId];

    metadata = UnitMetadata{};

    metadata.unitId = unitId;
    metadata.ident = ident;
    metadata.displayName = unitNames[unitId].name;

    // ---------------------------------------------------------
    // Unit -> Flingy -> Sprite -> Image
    // ---------------------------------------------------------

    metadata.flingyId = static_cast<std::size_t>(unit.flingy);

    if (metadata.flingyId >= flingies.size()) {
        error = "Unit " + std::to_string(unitId) + " references invalid flingy " + std::to_string(metadata.flingyId);
        return false;
    }

    metadata.spriteId = static_cast<std::size_t>(flingies[metadata.flingyId].sprite);

    // Preserve legacy Sprite::image() fallback.
    metadata.imageIndex = 0;

    if (metadata.spriteId < sprites.size()) {
        metadata.imageIndex = static_cast<std::size_t>(sprites[metadata.spriteId].image);
    }

    if (metadata.imageIndex >= images.size()) {
        error = "Sprite " + std::to_string(metadata.spriteId) + " references invalid image " + std::to_string(metadata.imageIndex);
        return false;
    }

    const std::uint32_t grpReference = images[metadata.imageIndex].grp;
    if (grpReference == 0) {
        error = "Image " + std::to_string(metadata.imageIndex) + " has a null images.tbl reference";
        return false;
    }

    const std::size_t grpTableIndex = static_cast<std::size_t>(grpReference - 1);
    if (grpTableIndex >= imageNames.size()) {
        error = "Image " + std::to_string(metadata.imageIndex) + " references invalid images.tbl entry " + std::to_string(grpTableIndex);
        return false;
    }

    metadata.imageId = makeImageIdentifier(metadata.imageIndex, imageNames[grpTableIndex].name);

    // ---------------------------------------------------------
    // Portrait
    // ---------------------------------------------------------

    if (unit.portrait == PortraitNone) {
        metadata.portraitId = "portrait_tadvisor";
    } else {
        const std::size_t portraitIndex = static_cast<std::size_t>(unit.portrait);

        if (portraitIndex >= portraits.size()) {
            error = "Unit " + std::to_string(unitId) + " references invalid portrait " + std::to_string(portraitIndex);
            return false;
        }

        const std::uint32_t portraitReference = portraits[portraitIndex].videoIdle;

        if (portraitReference == 0) {
            error = "Portrait " + std::to_string(portraitIndex) + " has a null portdata.tbl reference";
            return false;
        }

        const std::size_t portraitTableIndex = static_cast<std::size_t>(portraitReference - 1);

        if (portraitTableIndex >= portraitNames.size()) {
            error = "Portrait " + std::to_string(portraitIndex) + " references invalid portdata.tbl entry " + std::to_string(portraitTableIndex);
            return false;
        }

        metadata.portraitId = makePortraitIdentifier(portraitNames[portraitTableIndex].name);
    }

    // ---------------------------------------------------------
    // Basic properties
    // ---------------------------------------------------------

    metadata.hitPoints = static_cast<int>(unit.hitPoints);

    metadata.building = (unit.specialAbilityFlags & BuildingFlag) != 0;

    metadata.airUnit = (unit.specialAbilityFlags & FlyerFlag) != 0;

    metadata.organic = (unit.specialAbilityFlags & OrganicFlag) != 0;

    metadata.landUnit = !metadata.airUnit;

    // ---------------------------------------------------------
    // Dimensions
    // ---------------------------------------------------------

    constexpr double TilePixels = 8.0;

    const int width = static_cast<int>(unit.dimensions.left) + static_cast<int>(unit.dimensions.right);

    const int height = static_cast<int>(unit.dimensions.up) + static_cast<int>(unit.dimensions.down);

    double tileWidth = static_cast<double>(width) / TilePixels;

    double tileHeight = static_cast<double>(height) / TilePixels;

    metadata.boxWidth = static_cast<int>(std::round(tileWidth * TilePixels));

    metadata.boxHeight = static_cast<int>(std::round(tileHeight * TilePixels));

    if (!metadata.building) {
        const double square = std::round(std::sqrt(tileWidth * tileHeight));

        tileWidth = square;
        tileHeight = square;
    }

    if (tileWidth < 1.0) {
        tileWidth = 1.0;
    }

    if (tileHeight < 1.0) {
        tileHeight = 1.0;
    }

    /*
     * Startool legacy passes the double to lg::integer(int),
     * so building dimensions are truncated here
     */
    metadata.tileWidth = static_cast<int>(tileWidth);

    metadata.tileHeight = static_cast<int>(tileHeight);

    // ---------------------------------------------------------
    // Sight / shadow
    // ---------------------------------------------------------

    metadata.sightRange = static_cast<int>(unit.sightRange) * 4;

    if (unit.elevationLevel >= 16) {
        metadata.shadowX = 15;
        metadata.shadowY = 15;
    }

    // ---------------------------------------------------------
    // Costs
    // ---------------------------------------------------------

    metadata.buildTime = static_cast<int>((static_cast<double>(unit.buildTime) / 24.0) * 6.0);

    metadata.mineralCost = static_cast<int>(unit.mineralCost);

    metadata.gasCost = static_cast<int>(unit.vespeneCost);

    // ---------------------------------------------------------
    // Sounds
    // ---------------------------------------------------------

    metadata.readySounds.clear();

    if (unitId < 106 && unit.readySound != SoundNone) {
        std::string readySound;

        if (!resolveSound(sfxData, soundNames, unit.readySound, readySound, error)) {
            return false;
        }

        metadata.readySounds.push_back(std::move(readySound));
    }

    if (!resolveSoundRange(sfxData, soundNames, unit.whatSoundStart, unit.whatSoundEnd, metadata.whatSounds, error)) {
        return false;
    }

    if (unitId < 106) {
        if (!resolveSoundRange(sfxData, soundNames, unit.yesSoundStart, unit.yesSoundEnd, metadata.yesSounds, error)) {
            return false;
        }

        if (!resolveSoundRange(sfxData, soundNames, unit.pissSoundStart, unit.pissSoundEnd, metadata.pissSounds, error)) {
            return false;
        }
    }

    return true;
}
