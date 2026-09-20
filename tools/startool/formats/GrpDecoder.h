// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef GRP_DECODER_H
#define GRP_DECODER_H

#include "GrpImage.h"

#include <filesystem>
#include <string>

class GrpDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            GrpImage &image,
            std::string &error
        ) const;

};

#endif // GRP_DECODER_H
