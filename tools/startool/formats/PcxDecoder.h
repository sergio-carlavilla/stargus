// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PCX_DECODER_H
#define PCX_DECODER_H

#include "PcxImage.h"

#include <filesystem>
#include <string>

class PcxDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            PcxImage &image,
            std::string &error
        ) const;

};

#endif // PCX_DECODER_H
