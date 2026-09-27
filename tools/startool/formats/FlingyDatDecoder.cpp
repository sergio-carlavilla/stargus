// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "FlingyDatDecoder.h"

#include "BinaryReader.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{

    constexpr std::size_t FlingyRecordSize = 15;

}

bool FlingyDatDecoder::decode(
    const std::filesystem::path &input,
    std::vector<FlingyDatRecord> &records,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open flingy.dat file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    if (data.empty()) {
        error = "flingy.dat is empty: " + input.string();
        return false;
    }

    if (data.size() % FlingyRecordSize != 0) {
        error = "flingy.dat size is not divisible by 15 bytes: " + input.string();
        return false;
    }

    const std::size_t recordCount = data.size() / FlingyRecordSize;

    records.clear();
    records.resize(recordCount);

    // flingy.dat is stored column-wise
    // The first column contains one uint16 sprite ID per record
    const std::size_t spriteOffset = 0;

    for (std::size_t index = 0; index < recordCount; ++index) {
        records[index].sprite = BinaryReader::readLe16(data, spriteOffset + index * 2);
    }

    return true;
}
