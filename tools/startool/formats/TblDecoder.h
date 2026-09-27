// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef TBL_DECODER_H
#define TBL_DECODER_H

#include <filesystem>
#include <string>
#include <vector>

struct TblEntry
{
    std::string name;
};

class TblDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            std::vector<TblEntry> &entries,
            std::string &error
        ) const;

};

#endif // TBL_DECODER_H
