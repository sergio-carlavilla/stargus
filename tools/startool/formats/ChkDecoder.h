// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef CHK_DECODER_H
#define CHK_DECODER_H

#include "ChkMap.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

class ChkDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            ChkMap &map,
            std::string &error
        ) const;

        bool decode(
            const std::vector<std::uint8_t> &data,
            ChkMap &map,
            std::string &error
        ) const;

};

#endif // CHK_DECODER_H
