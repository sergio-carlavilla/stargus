// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef DDS_DECODER_H
#define DDS_DECODER_H

#include "DdsImage.h"

#include <filesystem>
#include <string>

/*
 * Decodes the top mipmap level of a DirectDraw Surface. Supported pixel
 * formats are the ones StarCraft: Remastered uses for its SD interface
 * textures: DXT1, DXT3, DXT5 and uncompressed 24/32-bit RGB(A) described by
 * channel masks. DX10 headers, cube maps and volume textures are rejected
 */
class DdsDecoder
{

    public:
        bool decode(
            const std::filesystem::path &input,
            DdsImage &image,
            std::string &error
        ) const;

};

#endif // DDS_DECODER_H
