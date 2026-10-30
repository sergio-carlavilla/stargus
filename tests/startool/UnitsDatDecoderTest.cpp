// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "formats/UnitsDatDecoder.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    struct TemporaryDirectory
    {
        std::filesystem::path path;

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    };

    struct HitPointCase
    {
        std::uint32_t raw;
        std::uint32_t expected;
    };

    constexpr std::array<HitPointCase, 16> HitPointCases{{
        {0x0186A000U, 100000U}, // Mineral fields, beacons and map objects.
        {0x00000000U, 0U},
        {0x00000001U, 0U},      // Auxiliary entries use a fractional placeholder.
        {0x000000FFU, 0U},
        {0x00000100U, 1U},
        {0x00002800U, 40U},
        {0x00005000U, 80U},
        {0x00006400U, 100U},
        {0x00019000U, 400U},
        {0x0000FF00U, 255U},
        {0x00010000U, 256U},
        {0x00FFFF00U, 65535U},
        {0x01000000U, 65536U},
        {0x01020304U, 66051U},  // Detects byte order and fractional-byte errors.
        {0xFFFFFFFFU, 16777215U},
        {0x000028FFU, 40U},
    }};

    void writeLe16(std::vector<unsigned char> &data, std::size_t offset, std::uint16_t value)
    {
        data[offset] = static_cast<unsigned char>(value);
        data[offset + 1] = static_cast<unsigned char>(value >> 8);
    }

    void writeLe32(std::vector<unsigned char> &data, std::size_t offset, std::uint32_t value)
    {
        for (unsigned byte = 0; byte != 4; ++byte) {
            data[offset + byte] = static_cast<unsigned char>(value >> (byte * 8));
        }
    }

    bool checkLayout(const std::filesystem::path &directory, bool broodWar)
    {
        constexpr std::size_t HitPointsOffset = 3156;
        constexpr std::size_t LastUnit = 227;
        const std::size_t mineralOffset = broodWar ? 14860 : 14404;
        const std::size_t sightOffset = broodWar ? 8172 : 7716;
        std::vector<unsigned char> data(broodWar ? 19876 : 19192, 0);
        for (std::size_t index = 0; index < HitPointCases.size(); ++index) {
            writeLe32(data, HitPointsOffset + index * 4, HitPointCases[index].raw);
        }
        data[LastUnit] = 7;
        data[sightOffset + LastUnit] = 17;
        writeLe16(data, mineralOffset + LastUnit * 2, 513);
        writeLe16(data, mineralOffset + 228 * 2 + LastUnit * 2, 32767);
        writeLe16(data, mineralOffset + 228 * 4 + LastUnit * 2, 65535);

        const std::string label = broodWar ? "broodwar" : "classic";
        const auto file = directory / (label + ".dat");
        {
            std::ofstream stream(file, std::ios::binary);
            stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
            if (!stream) {
                std::cerr << "Could not write " << file << '\n';
                return false;
            }
        }
        std::vector<UnitsDatRecord> records;
        std::string error;
        if (!UnitsDatDecoder{}.decode(file, records, error) || records.size() != 228) {
            std::cerr << label << ": " << error << '\n';
            return false;
        }
        bool passed = true;
        for (std::size_t index = 0; index < HitPointCases.size(); ++index) {
            if (records[index].hitPoints != HitPointCases[index].expected) {
                std::cerr << label << " HP case " << index << ": expected "
                    << HitPointCases[index].expected << ", got " << records[index].hitPoints << '\n';
                passed = false;
            }
        }
        const auto &last = records[LastUnit];
        if (last.flingy != 7 || last.sightRange != 17 || last.mineralCost != 513 ||
            last.vespeneCost != 32767 || last.buildTime != 65535) {
            std::cerr << label << ": column alignment changed\n";
            passed = false;
        }
        if (passed) {
            std::cout << label << ": PASS 16 hit-point cases and column alignment\n";
        }
        return passed;
    }
}

int main()
{
    std::error_code error;
    const auto temporaryRoot = std::filesystem::temp_directory_path(error);
    if (error) {
        std::cerr << error.message() << '\n';
        return 1;
    }
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directoryPath = temporaryRoot / ("startool-units-dat-test-" + std::to_string(stamp));
    if (!std::filesystem::create_directory(directoryPath, error) || error) {
        std::cerr << "Could not create temporary directory: " << error.message() << '\n';
        return 1;
    }
    TemporaryDirectory directory{directoryPath};
    const bool classic = checkLayout(directory.path, false);
    const bool broodWar = checkLayout(directory.path, true);
    return classic && broodWar ? 0 : 1;
}
