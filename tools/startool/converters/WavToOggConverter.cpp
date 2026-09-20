// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "WavToOggConverter.h"

#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace
{

    struct WavInfo
    {
        std::uint16_t format = 0;
        std::uint16_t channels = 0;
        std::uint32_t sampleRate = 0;
        std::uint16_t blockAlign = 0;
        std::uint16_t bitsPerSample = 0;

        std::streampos dataOffset = 0;
        std::uint32_t dataSize = 0;
    };

    bool readBytes(std::istream &input, char *buffer, std::size_t size)
    {
        input.read(buffer, static_cast<std::streamsize>(size));
        return input.good();
    }

    bool readUInt16(std::istream &input, std::uint16_t &value)
    {
        std::array<unsigned char, 2> bytes{};

        input.read(reinterpret_cast<char *>(bytes.data()), bytes.size());

        if (!input) {
            return false;
        }

        value =
            static_cast<std::uint16_t>(bytes[0]) |
            static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(bytes[1]) << 8
            );

        return true;
    }

    bool readUInt32(
        std::istream &input,
        std::uint32_t &value
    )
    {
        std::array<unsigned char, 4> bytes{};

        input.read(reinterpret_cast<char *>(bytes.data()), bytes.size());

        if (!input) {
            return false;
        }

        value =
            static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8) |
            (static_cast<std::uint32_t>(bytes[2]) << 16) |
            (static_cast<std::uint32_t>(bytes[3]) << 24);

        return true;
    }

    bool readWavInfo(
        std::ifstream &input,
        WavInfo &info,
        std::string &error
    )
    {
        char riff[4];
        if (!readBytes(input, riff, sizeof(riff)) || std::string(riff, sizeof(riff)) != "RIFF") {
            error = "Input file is not a RIFF file";
            return false;
        }

        std::uint32_t riffSize = 0;
        if (!readUInt32(input, riffSize)) {
            error = "Could not read RIFF size";
            return false;
        }

        char wave[4];
        if (!readBytes(input, wave, sizeof(wave)) || std::string(wave, sizeof(wave)) != "WAVE") {
            error = "Input file is not a WAVE file";
            return false;
        }

        bool formatFound = false;
        bool dataFound = false;

        while (input && (!formatFound || !dataFound)) {
            char chunkId[4];

            if (!readBytes(input, chunkId, sizeof(chunkId))) {
                break;
            }

            std::uint32_t chunkSize = 0;

            if (!readUInt32(input, chunkSize)) {
                break;
            }

            const std::string id(chunkId, sizeof(chunkId));

            if (id == "fmt ") {
                if (chunkSize < 16) {
                    error = "Invalid WAVE format chunk";
                    return false;
                }

                std::uint32_t byteRate = 0;

                if (!readUInt16(input, info.format) ||
                    !readUInt16(input, info.channels) ||
                    !readUInt32(input, info.sampleRate) ||
                    !readUInt32(input, byteRate) ||
                    !readUInt16(input, info.blockAlign) ||
                    !readUInt16(input, info.bitsPerSample))
                {
                    error = "Could not read WAVE format";
                    return false;
                }

                if (chunkSize > 16) {
                    input.seekg(
                        static_cast<std::streamoff>(chunkSize - 16),
                        std::ios::cur
                    );
                }

                formatFound = true;
            } else if (id == "data") {
                info.dataOffset = input.tellg();
                info.dataSize = chunkSize;

                input.seekg(
                    static_cast<std::streamoff>(chunkSize),
                    std::ios::cur
                );

                dataFound = true;
            } else {
                input.seekg(
                    static_cast<std::streamoff>(chunkSize),
                    std::ios::cur
                );
            }

            if (chunkSize % 2 != 0) {
                input.seekg(1, std::ios::cur);
            }
        }

        if (!formatFound) {
            error = "WAVE format chunk was not found";
            return false;
        }

        if (!dataFound) {
            error = "WAVE data chunk was not found";
            return false;
        }

        if (info.format != 1) {
            error = "Only PCM WAVE files are currently supported";
            return false;
        }

        if (info.bitsPerSample != 16) {
            error = "Only 16-bit PCM WAVE files are currently supported";
            return false;
        }

        if (info.channels == 0) {
            error = "WAVE file has no audio channels";
            return false;
        }

        const std::uint16_t expectedBlockAlign =
            static_cast<std::uint16_t>(
                info.channels *
                (info.bitsPerSample / 8)
            );

        if (info.blockAlign != expectedBlockAlign) {
            error = "Invalid WAVE block alignment";
            return false;
        }

        if (info.dataSize % info.blockAlign != 0) {
            error = "WAVE data size is not aligned to audio frames";
            return false;
        }

        return true;
    }

}

bool WavToOggConverter::convert(
    const std::filesystem::path &inputPath,
    const std::filesystem::path &outputPath,
    std::string &error
) const
{
    std::ifstream input(
        inputPath,
        std::ios::binary
    );

    if (!input) {
        error = "Could not open input WAVE file: " + inputPath.string();
        return false;
    }

    WavInfo wavInfo;

    if (!readWavInfo(input, wavInfo, error)) {
        return false;
    }

    input.clear();
    input.seekg(wavInfo.dataOffset);

    if (!input) {
        error = "Could not seek to WAVE audio data";
        return false;
    }

    std::ofstream output(
        outputPath,
        std::ios::binary
    );

    if (!output) {
        error = "Could not open output OGG file: " + outputPath.string();
        return false;
    }

    vorbis_info vorbisInfo;
    vorbis_info_init(&vorbisInfo);

    if (vorbis_encode_init_vbr(&vorbisInfo, wavInfo.channels, wavInfo.sampleRate, 0.4f) != 0) {
        vorbis_info_clear(&vorbisInfo);

        error = "Could not initialize Vorbis encoder";
        return false;
    }

    vorbis_comment vorbisComment;
    vorbis_comment_init(&vorbisComment);

    vorbis_comment_add_tag(
        &vorbisComment,
        const_cast<char *>("ENCODER"),
        const_cast<char *>("Startool 4")
    );

    vorbis_dsp_state vorbisState;
    if (vorbis_analysis_init(&vorbisState, &vorbisInfo) != 0) {
        vorbis_comment_clear(&vorbisComment);
        vorbis_info_clear(&vorbisInfo);

        error = "Could not initialize Vorbis analysis state";
        return false;
    }

    vorbis_block vorbisBlock;
    if (vorbis_block_init(&vorbisState, &vorbisBlock) != 0) {
        vorbis_dsp_clear(&vorbisState);
        vorbis_comment_clear(&vorbisComment);
        vorbis_info_clear(&vorbisInfo);

        error = "Could not initialize Vorbis block";
        return false;
    }

    ogg_stream_state oggStream;

    if (ogg_stream_init(&oggStream, 1) != 0) {
        vorbis_block_clear(&vorbisBlock);
        vorbis_dsp_clear(&vorbisState);
        vorbis_comment_clear(&vorbisComment);
        vorbis_info_clear(&vorbisInfo);

        error = "Could not initialize OGG stream";
        return false;
    }

    const auto cleanup =
        [&]()
        {
            ogg_stream_clear(&oggStream);
            vorbis_block_clear(&vorbisBlock);
            vorbis_dsp_clear(&vorbisState);
            vorbis_comment_clear(&vorbisComment);
            vorbis_info_clear(&vorbisInfo);
        };

    const auto writePage =
        [&](const ogg_page &page)
        {
            output.write(
                reinterpret_cast<const char *>(page.header),
                page.header_len
            );

            output.write(
                reinterpret_cast<const char *>(page.body),
                page.body_len
            );

            return output.good();
        };

    ogg_packet header;
    ogg_packet headerComment;
    ogg_packet headerCode;

    if (vorbis_analysis_headerout(&vorbisState, &vorbisComment, &header, &headerComment, &headerCode) != 0) {
        cleanup();

        error = "Could not create Vorbis headers";
        return false;
    }

    ogg_stream_packetin(
        &oggStream,
        &header
    );

    ogg_stream_packetin(
        &oggStream,
        &headerComment
    );

    ogg_stream_packetin(
        &oggStream,
        &headerCode
    );

    ogg_page page;

    while (ogg_stream_flush(&oggStream, &page)) {
        if (!writePage(page)) {
            cleanup();

            error = "Could not write OGG headers";
            return false;
        }
    }

    const auto processVorbisBlocks = [&]() {
        while (vorbis_analysis_blockout(&vorbisState, &vorbisBlock) == 1) {
            vorbis_analysis(&vorbisBlock, nullptr);

            vorbis_bitrate_addblock(&vorbisBlock);

            ogg_packet packet;

            while (vorbis_bitrate_flushpacket(&vorbisState, &packet)) {
                ogg_stream_packetin(&oggStream, &packet);

                while (ogg_stream_pageout(&oggStream, &page)) {
                    if (!writePage(page)) {
                        return false;
                    }
                }
            }
        }

        return true;
    };

    constexpr std::size_t framesPerBuffer = 4096;

    const std::size_t bytesPerFrame = wavInfo.blockAlign;

    std::vector<unsigned char> audioBuffer(framesPerBuffer * bytesPerFrame);
    std::uint32_t remainingBytes = wavInfo.dataSize;

    while (remainingBytes > 0) {
        const std::size_t framesToRead = std::min<std::size_t>(framesPerBuffer, remainingBytes / bytesPerFrame);
        const std::size_t bytesToRead = framesToRead * bytesPerFrame;

        input.read(
            reinterpret_cast<char *>(audioBuffer.data()),
            static_cast<std::streamsize>(bytesToRead)
        );

        if (input.gcount() != static_cast<std::streamsize>(bytesToRead)) {
            cleanup();

            error = "Could not read complete WAVE audio data";
            return false;
        }

        float **vorbisBuffer = vorbis_analysis_buffer(&vorbisState, static_cast<int>(framesToRead) );
        for (std::size_t frame = 0; frame < framesToRead; ++frame) {
            for (std::size_t channel = 0; channel < wavInfo.channels; ++channel) {
                const std::size_t sampleOffset = (frame * bytesPerFrame) + (channel * 2);

                const std::uint16_t sampleValue =
                    static_cast<std::uint16_t>(audioBuffer[sampleOffset]) |
                    static_cast<std::uint16_t>(static_cast<std::uint16_t>(audioBuffer[sampleOffset + 1]) << 8);

                const std::int16_t sample = static_cast<std::int16_t>(sampleValue);
                vorbisBuffer[channel][frame] = static_cast<float>(sample) / 32768.0f;
            }
        }

        vorbis_analysis_wrote(&vorbisState, static_cast<int>(framesToRead));

        if (!processVorbisBlocks()) {
            cleanup();

            error = "Could not write encoded OGG data";
            return false;
        }

        remainingBytes -= static_cast<std::uint32_t>(bytesToRead);
    }

    vorbis_analysis_wrote(&vorbisState, 0);

    if (!processVorbisBlocks()) {
        cleanup();

        error = "Could not finalize OGG data";
        return false;
    }

    while (ogg_stream_flush(&oggStream, &page)) {
        if (!writePage(page)) {
            cleanup();

            error = "Could not finalize OGG stream";
            return false;
        }
    }

    cleanup();

    if (!output.good()) {
        error = "Could not finish writing OGG file";
        return false;
    }

    return true;
}
