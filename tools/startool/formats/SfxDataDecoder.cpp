// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SfxDataDecoder.h"

#include "BinaryReader.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{

    constexpr std::size_t SfxDataRecordSize = 9;

}

bool SfxDataDecoder::decode(
    const std::filesystem::path &input,
    std::vector<SfxDataRecord> &records,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open sfxdata.dat file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    if (data.empty()) {
        error = "sfxdata.dat is empty: " + input.string();
        return false;
    }

    if (data.size() % SfxDataRecordSize != 0) {
        error = "sfxdata.dat size is not divisible by 9 bytes: " + input.string();
        return false;
    }

    const std::size_t recordCount = data.size() / SfxDataRecordSize;

    const std::size_t soundFileOffset = 0;

    const std::size_t unknown1Offset = soundFileOffset + recordCount * 4;

    const std::size_t unknown2Offset = unknown1Offset + recordCount;

    const std::size_t unknown3Offset = unknown2Offset + recordCount;

    const std::size_t unknown4Offset = unknown3Offset + recordCount * 2;

    records.clear();
    records.resize(recordCount);

    for (std::size_t index = 0; index < recordCount; ++index) {
        SfxDataRecord &record = records[index];

        record.soundFile = BinaryReader::readLe32(data, soundFileOffset + index * 4);

        record.unknown1 = data[unknown1Offset + index];

        record.unknown2 = data[unknown2Offset + index];

        record.unknown3 = BinaryReader::readLe16(data, unknown3Offset + index * 2);

        record.unknown4 = data[unknown4Offset + index];
    }

    return true;
}
