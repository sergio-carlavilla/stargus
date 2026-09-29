// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "formats/ImagesDatDecoder.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    void writeLe32At(
        std::vector<unsigned char> &data,
        std::size_t offset,
        std::uint32_t value
    )
    {
        data[offset] =
            static_cast<unsigned char>(value & 0xff);
        data[offset + 1] =
            static_cast<unsigned char>((value >> 8) & 0xff);
        data[offset + 2] =
            static_cast<unsigned char>((value >> 16) & 0xff);
        data[offset + 3] =
            static_cast<unsigned char>((value >> 24) & 0xff);
    }

    bool writeBytes(
        const std::filesystem::path &path,
        const std::vector<unsigned char> &data
    )
    {
        std::ofstream output(path, std::ios::binary);

        if (!output) {
            return false;
        }

        output.write(
            reinterpret_cast<const char *>(data.data()),
            static_cast<std::streamsize>(data.size())
        );

        return static_cast<bool>(output);
    }
}

int main()
{
    constexpr std::size_t RecordCount = 2;

    std::vector<unsigned char> data(
        RecordCount * 38,
        0
    );

    const std::size_t grpOffset = 0;
    const std::size_t gfxTurnsOffset = 4 * RecordCount;
    const std::size_t clickableOffset = 5 * RecordCount;
    const std::size_t useFullIscriptOffset = 6 * RecordCount;
    const std::size_t drawIfCloakedOffset = 7 * RecordCount;
    const std::size_t drawFunctionOffset = 8 * RecordCount;
    const std::size_t remappingOffset = 9 * RecordCount;
    const std::size_t iscriptOffset = 10 * RecordCount;
    const std::size_t shieldOverlayOffset = 14 * RecordCount;
    const std::size_t attackOverlayOffset = 18 * RecordCount;
    const std::size_t damageOverlayOffset = 22 * RecordCount;
    const std::size_t specialOverlayOffset = 26 * RecordCount;
    const std::size_t landingDustOverlayOffset = 30 * RecordCount;
    const std::size_t liftOffDustOverlayOffset = 34 * RecordCount;

    writeLe32At(data, grpOffset, 3);
    writeLe32At(data, grpOffset + 4, 7);

    data[gfxTurnsOffset] = 1;
    data[clickableOffset] = 1;
    data[useFullIscriptOffset + 1] = 1;
    data[drawIfCloakedOffset + 1] = 1;

    data[drawFunctionOffset] = 9;
    data[drawFunctionOffset + 1] = 10;

    data[remappingOffset] = 4;
    data[remappingOffset + 1] = 2;

    writeLe32At(data, iscriptOffset, 0x01020304u);
    writeLe32At(data, iscriptOffset + 4, 0x11121314u);

    writeLe32At(data, shieldOverlayOffset, 21);
    writeLe32At(data, shieldOverlayOffset + 4, 22);

    writeLe32At(data, attackOverlayOffset, 31);
    writeLe32At(data, attackOverlayOffset + 4, 32);

    writeLe32At(data, damageOverlayOffset, 41);
    writeLe32At(data, damageOverlayOffset + 4, 42);

    writeLe32At(data, specialOverlayOffset, 51);
    writeLe32At(data, specialOverlayOffset + 4, 52);

    writeLe32At(data, landingDustOverlayOffset, 61);
    writeLe32At(data, landingDustOverlayOffset + 4, 62);

    writeLe32At(data, liftOffDustOverlayOffset, 71);
    writeLe32At(data, liftOffDustOverlayOffset + 4, 72);

    std::error_code filesystemError;

    const std::filesystem::path root =
        std::filesystem::temp_directory_path(
            filesystemError
        ) /
        "startool-images-dat-decoder-test";

    if (filesystemError) {
        std::cerr << filesystemError.message() << '\n';
        return 1;
    }

    std::filesystem::remove_all(root, filesystemError);
    filesystemError.clear();
    std::filesystem::create_directories(root, filesystemError);

    const std::filesystem::path validPath =
        root / "images.dat";

    if (!writeBytes(validPath, data)) {
        std::cerr << "Could not write images.dat fixture\n";
        return 1;
    }

    ImagesDatDecoder decoder;
    std::vector<ImagesDatRecord> records;
    std::string error;

    if (!decoder.decode(
        validPath,
        records,
        error
    )) {
        std::cerr
            << "Valid images.dat failed: "
            << error
            << '\n';
        return 1;
    }

    if (records.size() != RecordCount) {
        std::cerr
            << "Expected 2 images.dat records, got "
            << records.size()
            << '\n';
        return 1;
    }

    const ImagesDatRecord &first = records[0];
    const ImagesDatRecord &second = records[1];

    if (
        first.grp != 3 ||
        !first.gfxTurns ||
        !first.clickable ||
        first.useFullIscript ||
        first.drawIfCloaked ||
        first.drawFunction != 9 ||
        first.remapping != 4 ||
        first.iscript != 0x01020304u ||
        first.shieldOverlay != 21 ||
        first.attackOverlay != 31 ||
        first.damageOverlay != 41 ||
        first.specialOverlay != 51 ||
        first.landingDustOverlay != 61 ||
        first.liftOffDustOverlay != 71
    ) {
        std::cerr << "First images.dat record decoded incorrectly\n";
        return 1;
    }

    if (
        second.grp != 7 ||
        second.gfxTurns ||
        second.clickable ||
        !second.useFullIscript ||
        !second.drawIfCloaked ||
        second.drawFunction != 10 ||
        second.remapping != 2 ||
        second.iscript != 0x11121314u ||
        second.shieldOverlay != 22 ||
        second.attackOverlay != 32 ||
        second.damageOverlay != 42 ||
        second.specialOverlay != 52 ||
        second.landingDustOverlay != 62 ||
        second.liftOffDustOverlay != 72
    ) {
        std::cerr << "Second images.dat record decoded incorrectly\n";
        return 1;
    }

    const std::filesystem::path invalidPath =
        root / "invalid.dat";

    if (!writeBytes(
        invalidPath,
        std::vector<unsigned char>(37, 0)
    )) {
        std::cerr << "Could not write invalid images.dat fixture\n";
        return 1;
    }

    records.clear();
    error.clear();

    if (decoder.decode(
        invalidPath,
        records,
        error
    )) {
        std::cerr
            << "Malformed images.dat was accepted\n";
        return 1;
    }

    std::filesystem::remove_all(root, filesystemError);

    return 0;
}
