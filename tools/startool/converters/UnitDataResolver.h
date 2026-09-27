// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef UNIT_DATA_RESOLVER_H
#define UNIT_DATA_RESOLVER_H

#include "UnitMetadata.h"
#include "formats/FlingyDatDecoder.h"
#include "formats/ImagesDatDecoder.h"
#include "formats/PortDataDecoder.h"
#include "formats/SfxDataDecoder.h"
#include "formats/SpritesDatDecoder.h"
#include "formats/TblDecoder.h"
#include "formats/UnitsDatDecoder.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct UnitDataFiles
{
    std::filesystem::path unitsDat;
    std::filesystem::path flingyDat;
    std::filesystem::path spritesDat;

    std::filesystem::path imagesDat;
    std::filesystem::path imagesTbl;

    std::filesystem::path portdataDat;
    std::filesystem::path portdataTbl;

    std::filesystem::path sfxdataDat;
    std::filesystem::path sfxdataTbl;

    std::filesystem::path statTxtTbl;
};

class UnitDataResolver
{

    public:
        bool load(
            const UnitDataFiles &files,
            std::string &error
        );

        bool resolve(
            std::size_t unitId,
            const std::string &ident,
            UnitMetadata &metadata,
            std::string &error
        ) const;

        bool resolve(
            const UnitDataFiles &files,
            std::size_t unitId,
            const std::string &ident,
            UnitMetadata &metadata,
            std::string &error
        ) const;

    private:
        bool mLoaded = false;

        std::vector<UnitsDatRecord> mUnits;
        std::vector<FlingyDatRecord> mFlingies;
        std::vector<SpritesDatRecord> mSprites;
        std::vector<ImagesDatRecord> mImages;
        std::vector<PortDataRecord> mPortraits;
        std::vector<SfxDataRecord> mSfxData;

        std::vector<TblEntry> mImageNames;
        std::vector<TblEntry> mPortraitNames;
        std::vector<TblEntry> mSoundNames;
        std::vector<TblEntry> mUnitNames;

};

#endif // UNIT_DATA_RESOLVER_H
