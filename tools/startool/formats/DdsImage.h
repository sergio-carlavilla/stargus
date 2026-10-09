// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef DDS_IMAGE_H
#define DDS_IMAGE_H

#include <cstdint>
#include <vector>

struct DdsImage
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    // Top mipmap level as straight RGBA, 4 bytes per pixel, row-major
    std::vector<std::uint8_t> rgba;
};

#endif // DDS_IMAGE_H
