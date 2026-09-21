// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "TilesetToLuaConverter.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

    constexpr std::size_t Cv5GroupSize = 52;
    constexpr std::size_t Cv5MegatileReferencesOffset = 20;
    constexpr std::size_t Cv5MegatileReferenceCount = 16;

    constexpr std::size_t Vf4RecordSize = 32;
    constexpr std::size_t Vf4FlagCount = 16;

    std::uint16_t readLittleEndian16(
        const std::vector<std::uint8_t> &data,
        std::size_t offset
    )
    {
        return static_cast<std::uint16_t>(data[offset]) |
            (static_cast<std::uint16_t>(data[offset + 1]) << 8);
    }

    bool readFile(
        const std::filesystem::path &path,
        std::vector<std::uint8_t> &data,
        std::string &error
    )
    {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            error = "Could not open tileset resource: " + path.string();
            return false;
        }

        data.assign(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );

        if (!input.eof() && input.fail()) {
            error = "Could not read tileset resource: " + path.string();
            return false;
        }

        return true;
    }

    std::string quote(const std::string &value)
    {
        return '"' + value + '"';
    }

    std::string table(const std::string &content)
    {
        return '{' + content + '}';
    }

    std::string join(const std::vector<std::string> &values)
    {
        std::string result;

        for (std::size_t index = 0; index < values.size(); ++index) {
            if (index != 0) {
                result += ", ";
            }

            result += values[index];
        }

        return result;
    }

}

bool TilesetToLuaConverter::convert(
    const std::filesystem::path &cv5Input,
    const std::filesystem::path &vf4Input,
    const std::filesystem::path &output,
    const std::string &tilesetName,
    const std::string &image,
    std::string &error
) const
{
    std::vector<std::uint8_t> cv5;
    std::vector<std::uint8_t> vf4;

    if (!readFile(cv5Input, cv5, error)) {
        return false;
    }

    if (!readFile(vf4Input, vf4, error)) {
        return false;
    }

    if (cv5.empty() || cv5.size() % Cv5GroupSize != 0) {
        error = "Invalid CV5 size: " + cv5Input.string();
        return false;
    }

    if (vf4.empty() || vf4.size() % Vf4RecordSize != 0) {
        error = "Invalid VF4 size: " + vf4Input.string();
        return false;
    }

    const std::size_t cv5GroupCount = cv5.size() / Cv5GroupSize;
    const std::size_t vf4RecordCount = vf4.size() / Vf4RecordSize;

    std::vector<std::string> tileSlots;
    tileSlots.reserve(cv5GroupCount);

    for (std::size_t groupIndex = 0; groupIndex < cv5GroupCount; ++groupIndex) {
        const std::size_t groupOffset = groupIndex * Cv5GroupSize;

        std::vector<std::string> tileSolids;
        tileSolids.reserve(Cv5MegatileReferenceCount * 2);

        for (std::size_t referenceIndex = 0; referenceIndex < Cv5MegatileReferenceCount; ++referenceIndex) {
            const std::size_t referenceOffset = groupOffset + Cv5MegatileReferencesOffset + referenceIndex * 2;

            const std::uint16_t megatileReference = readLittleEndian16(cv5, referenceOffset);

            if (megatileReference >= vf4RecordCount) {
                error = "CV5 megatile reference exceeds VF4 data at group " + std::to_string(groupIndex) + ": " + std::to_string(megatileReference);
                return false;
            }

            const std::size_t vf4Offset = static_cast<std::size_t>(megatileReference) * Vf4RecordSize;

            std::string passableFlags;
            passableFlags.reserve(Vf4FlagCount);

            for (std::size_t flagIndex = 0; flagIndex < Vf4FlagCount; ++flagIndex) {
                const std::uint16_t flags = readLittleEndian16(
                    vf4,
                    vf4Offset + flagIndex * 2
                );

                passableFlags += (flags & 1) != 0 ? 'p' : 'u';
            }

            tileSolids.push_back(std::to_string(megatileReference));
            tileSolids.push_back(table(quote(passableFlags)));
        }

        const std::string solid =
            quote("solid") +
            ", " +
            table(
                quote("light-grass") +
                ", " +
                quote("land") +
                ", " +
                table(join(tileSolids))
            ) +
            '\n';

        tileSlots.push_back(solid);
    }

    const std::string slots = table(join(tileSlots));

    const std::string lua =
        "DefineTileset(" +
        quote("name") +
        ", " +
        quote(tilesetName) +
        ", " +
        quote("size") +
        ", " +
        table("32, 32") +
        ", " +
        quote("image") +
        ", " +
        quote(image) +
        ", " +
        quote("slots") +
        ", " +
        slots +
        ")";

    std::ofstream outputStream(output, std::ios::binary);
    if (!outputStream) {
        error = "Could not open tileset Lua output: " + output.string();
        return false;
    }

    outputStream.write(lua.data(), static_cast<std::streamsize>(lua.size()));
    if (!outputStream) {
        error = "Could not write tileset Lua output: " + output.string();
        return false;
    }

    return true;
}
