// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PORT_DATA_DECODER_H
#define PORT_DATA_DECODER_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct PortDataRecord
{
    std::uint32_t videoIdle = 0;
    std::uint32_t videoTalking = 0;

    std::uint8_t changeIdle = 0;
    std::uint8_t changeTalking = 0;

    std::uint8_t unknownIdle = 0;
    std::uint8_t unknownTalking = 0;
};

class PortDataDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            std::vector<PortDataRecord> &records,
            std::string &error
        ) const;

};

#endif // PORT_DATA_DECODER_H
