// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef FLINGY_DAT_DECODER_H
#define FLINGY_DAT_DECODER_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct FlingyDatRecord
{
    std::uint16_t sprite = 0;
};

class FlingyDatDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            std::vector<FlingyDatRecord> &records,
            std::string &error
        ) const;

};

#endif // FLINGY_DAT_DECODER_H
