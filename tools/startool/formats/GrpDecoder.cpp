// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "GrpDecoder.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <vector>

namespace
{

    constexpr std::size_t GrpHeaderSize = 6;
    constexpr std::size_t GrpFrameHeaderSize = 8;

    std::uint16_t readLittleEndian16(
        const std::vector<unsigned char> &data,
        std::size_t offset
    )
    {
        return static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(data[offset]) |
            static_cast<std::uint16_t>(data[offset + 1]) << 8
        );
    }

    std::uint32_t readLittleEndian32(
        const std::vector<unsigned char> &data,
        std::size_t offset
    )
    {
        return
            static_cast<std::uint32_t>(data[offset]) |
            static_cast<std::uint32_t>(data[offset + 1]) << 8 |
            static_cast<std::uint32_t>(data[offset + 2]) << 16 |
            static_cast<std::uint32_t>(data[offset + 3]) << 24;
    }

}

bool GrpDecoder::decode(
    const std::filesystem::path &input,
    GrpImage &image,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open GRP file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    if (data.size() < GrpHeaderSize) {
        error = "GRP file is too small";
        return false;
    }

    const std::uint16_t frameCount = readLittleEndian16(data, 0);

    image.maximumWidth = readLittleEndian16(data, 2);
    image.maximumHeight = readLittleEndian16(data, 4);

    if (frameCount == 0) {
        error = "GRP file contains no frames";
        return false;
    }

    if (image.maximumWidth == 0 || image.maximumHeight == 0) {
        error = "GRP file has invalid maximum dimensions";
        return false;
    }

    if (
        frameCount >
        (
            std::numeric_limits<std::size_t>::max() -
            GrpHeaderSize
        ) / GrpFrameHeaderSize
    ) {
        error = "GRP frame table is too large";
        return false;
    }

    const std::size_t frameTableSize = static_cast<std::size_t>(frameCount) * GrpFrameHeaderSize;
    const std::size_t payloadStart = GrpHeaderSize + frameTableSize;

    if (payloadStart > data.size()) {
        error = "GRP frame table exceeds file size";
        return false;
    }

    image.frames.clear();
    image.frames.reserve(frameCount);

    for (std::size_t index = 0; index < frameCount; ++index) {
        const std::size_t offset = GrpHeaderSize + index * GrpFrameHeaderSize;

        GrpFrame frame;

        frame.xOffset = data[offset];
        frame.yOffset = data[offset + 1];
        frame.width = data[offset + 2];
        frame.height = data[offset + 3];

        frame.dataOffset = readLittleEndian32(data, offset + 4);

        if (frame.width == 0 || frame.height == 0) {
            error = "GRP frame " + std::to_string(index) + " has invalid dimensions";
            return false;
        }

        if (
            static_cast<std::uint32_t>(frame.xOffset) +
            static_cast<std::uint32_t>(frame.width) >
            image.maximumWidth
        ) {
            error = "GRP frame " + std::to_string(index) + " exceeds maximum width";
            return false;
        }

        if (
            static_cast<std::uint32_t>(frame.yOffset) +
            static_cast<std::uint32_t>(frame.height) >
            image.maximumHeight
        ) {
            error = "GRP frame " + std::to_string(index) + " exceeds maximum height";
            return false;
        }

        if (frame.dataOffset < payloadStart) {
            error = "GRP frame " + std::to_string(index) + " points inside the frame table";
            return false;
        }

        if (frame.dataOffset >= data.size()) {
            error = "GRP frame " + std::to_string(index) + " points outside the file";
            return false;
        }

        image.frames.push_back(std::move(frame));
    }

    std::set<std::uint32_t> uniqueOffsets;

    std::size_t uncompressedPayloadSize = 0;

    for (const GrpFrame &frame : image.frames) {
        if (!uniqueOffsets.insert(frame.dataOffset).second) {
            continue;
        }

        const std::size_t width = static_cast<std::size_t>(frame.width);
        const std::size_t height = static_cast<std::size_t>(frame.height);

        if (height > std::numeric_limits<std::size_t>::max() / width) {
            error = "GRP frame payload size is too large";
            return false;
        }

        const std::size_t frameSize = width * height;

        if (
            uncompressedPayloadSize >
            std::numeric_limits<std::size_t>::max() -
            frameSize
        ) {
            error = "GRP payload size is too large";
            return false;
        }

        uncompressedPayloadSize += frameSize;
    }

    const std::size_t firstOffset = static_cast<std::size_t>(image.frames.front().dataOffset);

    image.uncompressed = firstOffset <= data.size() && uncompressedPayloadSize == data.size() - firstOffset;

    if (!image.uncompressed) {
        error = "Compressed GRP files are not supported yet";
        return false;
    }

    for (std::size_t index = 0; index < image.frames.size(); ++index) {
        GrpFrame &frame = image.frames[index];

        const std::size_t frameSize =
            static_cast<std::size_t>(frame.width) *
            static_cast<std::size_t>(frame.height);

        const std::size_t dataOffset = static_cast<std::size_t>(frame.dataOffset);

        if (frameSize > data.size() || dataOffset > data.size() - frameSize) {
            error = "GRP frame " + std::to_string(index) + " payload exceeds file size";
            return false;
        }

        frame.pixels.assign(data.begin() + dataOffset, data.begin() + dataOffset + frameSize);
    }

    return true;
}
