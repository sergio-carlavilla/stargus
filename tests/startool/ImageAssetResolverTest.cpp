// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "converters/ImageAssetResolver.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    struct ImageFixture
    {
        std::string name;
        std::uint8_t drawFunction = 0;
        std::uint8_t remapping = 0;
        bool gfxTurns = false;
    };

    void appendLe16(
        std::vector<unsigned char> &data,
        std::uint16_t value
    )
    {
        data.push_back(
            static_cast<unsigned char>(value & 0xff)
        );
        data.push_back(
            static_cast<unsigned char>((value >> 8) & 0xff)
        );
    }

    void writeLe32At(
        std::vector<unsigned char> &data,
        std::size_t offset,
        std::uint32_t value
    )
    {
        data[offset] =
            static_cast<unsigned char>(value & 0xff);
        data[offset + 1] =
            static_cast<unsigned char>((value >> 8) & 0xff);
        data[offset + 2] =
            static_cast<unsigned char>((value >> 16) & 0xff);
        data[offset + 3] =
            static_cast<unsigned char>((value >> 24) & 0xff);
    }

    bool writeBytes(
        const std::filesystem::path &path,
        const std::vector<unsigned char> &data
    )
    {
        std::ofstream output(path, std::ios::binary);

        if (!output) {
            return false;
        }

        output.write(
            reinterpret_cast<const char *>(data.data()),
            static_cast<std::streamsize>(data.size())
        );

        return static_cast<bool>(output);
    }

    std::vector<unsigned char> makeTbl(
        const std::vector<ImageFixture> &fixtures
    )
    {
        std::vector<unsigned char> data;

        appendLe16(
            data,
            static_cast<std::uint16_t>(fixtures.size())
        );

        std::uint16_t offset =
            static_cast<std::uint16_t>(
                2 + fixtures.size() * 2
            );

        for (const ImageFixture &fixture : fixtures) {
            appendLe16(data, offset);

            offset = static_cast<std::uint16_t>(
                offset + fixture.name.size() + 1
            );
        }

        for (const ImageFixture &fixture : fixtures) {
            data.insert(
                data.end(),
                fixture.name.begin(),
                fixture.name.end()
            );
            data.push_back(0);
        }

        return data;
    }

    std::vector<unsigned char> makeImagesDat(
        const std::vector<ImageFixture> &fixtures
    )
    {
        const std::size_t count = fixtures.size();

        std::vector<unsigned char> data(
            count * 38,
            0
        );

        const std::size_t grpOffset = 0;
        const std::size_t gfxTurnsOffset = 4 * count;
        const std::size_t drawFunctionOffset = 8 * count;
        const std::size_t remappingOffset = 9 * count;

        for (
            std::size_t index = 0;
            index < count;
            ++index
        ) {
            writeLe32At(
                data,
                grpOffset + index * 4,
                static_cast<std::uint32_t>(index + 1)
            );

            data[gfxTurnsOffset + index] =
                fixtures[index].gfxTurns
                    ? 1
                    : 0;

            data[drawFunctionOffset + index] =
                fixtures[index].drawFunction;

            data[remappingOffset + index] =
                fixtures[index].remapping;
        }

        return data;
    }

    bool expectResolved(
        const ImageAssetResolver &resolver,
        const std::filesystem::path &dat,
        const std::filesystem::path &tbl,
        std::size_t index,
        const std::string &grpName,
        const std::string &palette,
        bool rgba,
        bool save,
        const std::string &pngOutput,
        const std::string &luaId,
        bool gfxTurns = false
    )
    {
        ImageAssetMetadata metadata;
        std::string error;

        if (!resolver.resolve(
            dat,
            tbl,
            index,
            metadata,
            error
        )) {
            std::cerr
                << "Resolver failed for image "
                << index
                << ": "
                << error
                << '\n';

            return false;
        }

        const std::string expectedLuaOutput =
            "luagen/images/" +
            luaId +
            ".lua";

        if (
            metadata.index != index ||
            metadata.grpName != grpName ||
            metadata.grpInput != "unit/" + grpName ||
            metadata.palette != palette ||
            metadata.rgba != rgba ||
            metadata.save != save ||
            metadata.pngOutput != pngOutput ||
            metadata.luaId != luaId ||
            metadata.luaOutput != expectedLuaOutput ||
            metadata.gfxTurns != gfxTurns
        ) {
            std::cerr
                << "Unexpected metadata for image "
                << index
                << '\n'
                << "  grpName:   "
                << metadata.grpName
                << '\n'
                << "  palette:   "
                << metadata.palette
                << '\n'
                << "  rgba:      "
                << metadata.rgba
                << '\n'
                << "  save:      "
                << metadata.save
                << '\n'
                << "  pngOutput: "
                << metadata.pngOutput
                << '\n'
                << "  luaId:     "
                << metadata.luaId
                << '\n'
                << "  luaOutput: "
                << metadata.luaOutput
                << '\n'
                << "  gfxTurns:  "
                << metadata.gfxTurns
                << '\n';

            return false;
        }

        return true;
    }
}

int main()
{
    const std::vector<ImageFixture> fixtures{
        {"Terran\\Marine.GRP", 0, 0, true},
        {"thingy\\fire1.grp", 9, 1, false},
        {"thingy\\fire2.grp", 9, 2, false},
        {"thingy\\fire3.grp", 9, 3, false},
        {"thingy\\fire4.grp", 9, 4, false},
        {"thingy\\shadow.grp", 10, 0, false},
        {"thingy\\tileset\\AshWorld\\lava.grp", 0, 0, false},
        {"terran\\tank.grp", 0, 0, false},
        {"neutral\\ion.grp", 0, 0, false},
        {"thingy\\blackx.grp", 0, 0, false},
        {"thingy\\fallback.grp", 9, 99, false},
        {"neutral\\geyser.grp", 0, 0, false}
    };

    std::error_code filesystemError;

    const std::filesystem::path root =
        std::filesystem::temp_directory_path(
            filesystemError
        ) /
        "startool-image-asset-resolver-test";

    if (filesystemError) {
        std::cerr << filesystemError.message() << '\n';
        return 1;
    }

    std::filesystem::remove_all(root, filesystemError);
    filesystemError.clear();
    std::filesystem::create_directories(root, filesystemError);

    const std::filesystem::path dat =
        root / "images.dat";

    const std::filesystem::path tbl =
        root / "images.tbl";

    if (
        !writeBytes(dat, makeImagesDat(fixtures)) ||
        !writeBytes(tbl, makeTbl(fixtures))
    ) {
        std::cerr << "Could not write image resolver fixtures\n";
        return 1;
    }

    const ImageAssetResolver resolver;

    if (!expectResolved(
        resolver,
        dat,
        tbl,
        0,
        "terran/marine.grp",
        "tunit",
        false,
        true,
        "graphics/unit/terran/marine.png",
        "image_0_terran_marine",
        true
    )) {
        return 1;
    }

    const char *remapPalettes[] = {
        "ofire",
        "gfire",
        "bfire",
        "bexpl"
    };

    for (
        std::size_t offset = 0;
        offset < 4;
        ++offset
    ) {
        const std::size_t index = offset + 1;
        const std::string number =
            std::to_string(index);

        if (!expectResolved(
            resolver,
            dat,
            tbl,
            index,
            "thingy/fire" + number + ".grp",
            remapPalettes[offset],
            true,
            true,
            "graphics/unit/thingy/fire" +
                number +
                "_" +
                remapPalettes[offset] +
                ".png",
            "image_" +
                std::to_string(index) +
                "_thingy_fire" +
                number
        )) {
            return 1;
        }
    }

    if (!expectResolved(
        resolver,
        dat,
        tbl,
        5,
        "thingy/shadow.grp",
        "tunit",
        false,
        false,
        "graphics/unit/thingy/shadow.png",
        "image_5_thingy_shadow"
    )) {
        return 1;
    }

    if (!expectResolved(
        resolver,
        dat,
        tbl,
        6,
        "thingy/tileset/ashworld/lava.grp",
        "ashworld",
        false,
        true,
        "graphics/unit/thingy/tileset/ashworld/lava.png",
        "image_6_thingy_tileset_ashworld_lava"
    )) {
        return 1;
    }

    if (!expectResolved(
        resolver,
        dat,
        tbl,
        7,
        "terran/tank.grp",
        "badlands",
        false,
        true,
        "graphics/unit/terran/tank.png",
        "image_7_terran_tank"
    )) {
        return 1;
    }

    if (!expectResolved(
        resolver,
        dat,
        tbl,
        8,
        "neutral/ion.grp",
        "platform",
        false,
        true,
        "graphics/unit/neutral/ion.png",
        "image_8_neutral_ion"
    )) {
        return 1;
    }

    if (!expectResolved(
        resolver,
        dat,
        tbl,
        9,
        "thingy/blackx.grp",
        "tunit",
        false,
        false,
        "graphics/unit/thingy/blackx.png",
        "image_9_thingy_blackx"
    )) {
        return 1;
    }

    if (!expectResolved(
        resolver,
        dat,
        tbl,
        10,
        "thingy/fallback.grp",
        "ofire",
        true,
        true,
        "graphics/unit/thingy/fallback_ofire.png",
        "image_10_thingy_fallback"
    )) {
        return 1;
    }

    if (!expectResolved(
        resolver,
        dat,
        tbl,
        11,
        "neutral/geyser.grp",
        "badlands",
        false,
        true,
        "graphics/unit/neutral/geyser.png",
        "image_11_neutral_geyser"
    )) {
        return 1;
    }

    ImageAssetMetadata metadata;
    std::string error;

    if (resolver.resolve(
        dat,
        tbl,
        fixtures.size(),
        metadata,
        error
    )) {
        std::cerr
            << "Out-of-range image index was accepted\n";
        return 1;
    }

    std::filesystem::remove_all(root, filesystemError);

    return 0;
}
