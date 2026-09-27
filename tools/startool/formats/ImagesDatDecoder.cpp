// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ImagesDatDecoder.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{

    constexpr std::size_t ImagesDatRecordSize = 38;

    std::uint32_t readLittleEndian32(
        const std::vector<unsigned char> &data,
        std::size_t offset
    )
    {
        return
            static_cast<std::uint32_t>(data[offset]) |
            static_cast<std::uint32_t>(data[offset + 1]) << 8 |
            static_cast<std::uint32_t>(data[offset + 2]) << 16 |
            static_cast<std::uint32_t>(data[offset + 3]) << 24;
    }

}

bool ImagesDatDecoder::decode(
    const std::filesystem::path &input,
    std::vector<ImagesDatRecord> &records,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open images.dat file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    if (data.empty()) {
        error = "images.dat is empty: " + input.string();
        return false;
    }

    if (data.size() % ImagesDatRecordSize != 0) {
        error = "images.dat size is not divisible by 38 bytes: " + input.string();
        return false;
    }

    const std::size_t recordCount = data.size() / ImagesDatRecordSize;

    const std::size_t grpOffset = 0;
    const std::size_t gfxTurnsOffset = 4 * recordCount;
    const std::size_t clickableOffset = 5 * recordCount;
    const std::size_t useFullIscriptOffset = 6 * recordCount;
    const std::size_t drawIfCloakedOffset = 7 * recordCount;
    const std::size_t drawFunctionOffset = 8 * recordCount;
    const std::size_t remappingOffset = 9 * recordCount;
    const std::size_t iscriptOffset = 10 * recordCount;
    const std::size_t shieldOverlayOffset = 14 * recordCount;
    const std::size_t attackOverlayOffset = 18 * recordCount;
    const std::size_t damageOverlayOffset = 22 * recordCount;
    const std::size_t specialOverlayOffset = 26 * recordCount;
    const std::size_t landingDustOverlayOffset = 30 * recordCount;
    const std::size_t liftOffDustOverlayOffset = 34 * recordCount;

    records.clear();
    records.resize(recordCount);

    for (std::size_t index = 0; index < recordCount; ++index) {
        ImagesDatRecord &record = records[index];

        record.grp = readLittleEndian32(data, grpOffset + index * 4);
        record.gfxTurns = data[gfxTurnsOffset + index] != 0;
        record.clickable = data[clickableOffset + index] != 0;
        record.useFullIscript = data[useFullIscriptOffset + index] != 0;
        record.drawIfCloaked = data[drawIfCloakedOffset + index] != 0;
        record.drawFunction = data[drawFunctionOffset + index];
        record.remapping = data[remappingOffset + index];
        record.iscript = readLittleEndian32(data, iscriptOffset + index * 4);
        record.shieldOverlay = readLittleEndian32(data, shieldOverlayOffset + index * 4);
        record.attackOverlay = readLittleEndian32(data, attackOverlayOffset + index * 4);
        record.damageOverlay = readLittleEndian32(data, damageOverlayOffset + index * 4);
        record.specialOverlay = readLittleEndian32(data, specialOverlayOffset + index * 4);
        record.landingDustOverlay = readLittleEndian32(data, landingDustOverlayOffset + index * 4);
        record.liftOffDustOverlay = readLittleEndian32(data, liftOffDustOverlayOffset + index * 4);
    }

    return true;
}
