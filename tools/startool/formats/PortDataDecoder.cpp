// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PortDataDecoder.h"

#include "BinaryReader.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{

    constexpr std::size_t PortDataRecordSize = 12;

}

bool PortDataDecoder::decode(
    const std::filesystem::path &input,
    std::vector<PortDataRecord> &records,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open portdata.dat file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    if (data.empty()) {
        error = "portdata.dat is empty: " + input.string();
        return false;
    }

    if (data.size() % PortDataRecordSize != 0) {
        error = "portdata.dat size is not divisible by 12 bytes: " + input.string();
        return false;
    }

    const std::size_t recordCount = data.size() / PortDataRecordSize;

    const std::size_t videoIdleOffset = 0;
    const std::size_t videoTalkingOffset = videoIdleOffset + recordCount * 4;

    const std::size_t changeIdleOffset = videoTalkingOffset + recordCount * 4;

    const std::size_t changeTalkingOffset = changeIdleOffset + recordCount;

    const std::size_t unknownIdleOffset = changeTalkingOffset + recordCount;

    const std::size_t unknownTalkingOffset = unknownIdleOffset + recordCount;

    records.clear();
    records.resize(recordCount);

    for (std::size_t index = 0; index < recordCount; ++index) {
        PortDataRecord &record = records[index];

        record.videoIdle = BinaryReader::readLe32(data, videoIdleOffset + index * 4);

        record.videoTalking = BinaryReader::readLe32(data, videoTalkingOffset + index * 4);

        record.changeIdle = data[changeIdleOffset + index];

        record.changeTalking = data[changeTalkingOffset + index];

        record.unknownIdle = data[unknownIdleOffset + index];

        record.unknownTalking = data[unknownTalkingOffset + index];
    }

    return true;
}
