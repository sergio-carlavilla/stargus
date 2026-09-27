// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SpritesDatDecoder.h"

#include "BinaryReader.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{

    constexpr std::size_t FirstRecordCount = 130;

    constexpr std::size_t FirstRecordSize = 4;
    constexpr std::size_t RemainingRecordSize = 7;

    constexpr std::size_t FirstBlockSize = FirstRecordCount * FirstRecordSize;

}

bool SpritesDatDecoder::decode(
    const std::filesystem::path &input,
    std::vector<SpritesDatRecord> &records,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open sprites.dat file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    if (data.size() < FirstBlockSize) {
        error = "sprites.dat is too small: " + input.string();
        return false;
    }

    const std::size_t remainingSize = data.size() - FirstBlockSize;
    if (remainingSize % RemainingRecordSize != 0) {
        error = "sprites.dat has an invalid size: " + input.string();
        return false;
    }

    const std::size_t recordCount = FirstRecordCount + remainingSize / RemainingRecordSize;

    records.clear();
    records.resize(recordCount);

    // The first column contains one uint16 images.dat ID
    // for every sprite
    const std::size_t imageOffset = 0;
    for (std::size_t index = 0; index < recordCount; ++index) {
        records[index].image = BinaryReader::readLe16(data, imageOffset + index * 2);
    }

    return true;
}
