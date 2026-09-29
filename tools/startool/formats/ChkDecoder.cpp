// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ChkDecoder.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

    std::uint16_t readLe16(
        const std::vector<std::uint8_t> &data,
        std::size_t offset
    )
    {
        return
            static_cast<std::uint16_t>(data[offset]) |
            static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(data[offset + 1]) << 8
            );
    }

    std::uint32_t readLe32(
        const std::vector<std::uint8_t> &data,
        std::size_t offset
    )
    {
        return
            static_cast<std::uint32_t>(data[offset]) |
            (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
            (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
            (static_cast<std::uint32_t>(data[offset + 3]) << 24);
    }

    bool rangeFits(
        std::size_t offset,
        std::size_t length,
        std::size_t size
    )
    {
        return offset <= size && length <= size - offset;
    }

    std::string sectionName(
        const std::vector<std::uint8_t> &data,
        std::size_t offset
    )
    {
        return std::string(
            reinterpret_cast<const char *>(data.data() + offset),
            4
        );
    }

    std::string readChkString(
        const std::vector<std::uint8_t> &data,
        std::size_t sectionStart,
        std::size_t sectionEnd,
        std::uint16_t stringOffset,
        bool &valid
    )
    {
        valid = false;

        const std::size_t begin =
            sectionStart + static_cast<std::size_t>(stringOffset);

        if (begin >= sectionEnd) {
            return {};
        }

        auto terminator = std::find(
            data.begin() + static_cast<std::ptrdiff_t>(begin),
            data.begin() + static_cast<std::ptrdiff_t>(sectionEnd),
            static_cast<std::uint8_t>(0)
        );

        if (terminator == data.begin() + static_cast<std::ptrdiff_t>(sectionEnd)) {
            return {};
        }

        std::string value(
            reinterpret_cast<const char *>(data.data() + begin),
            static_cast<std::size_t>(
                terminator -
                (data.begin() + static_cast<std::ptrdiff_t>(begin))
            )
        );

        value.erase(
            std::remove(value.begin(), value.end(), '\r'),
            value.end()
        );

        value.erase(
            std::remove(value.begin(), value.end(), '\n'),
            value.end()
        );

        valid = true;
        return value;
    }

}

bool ChkDecoder::decode(
    const std::filesystem::path &input,
    ChkMap &map,
    std::string &error
) const
{
    std::ifstream file(
        input,
        std::ios::binary
    );

    if (!file) {
        error = "Could not open CHK file: " + input.string();
        return false;
    }

    std::vector<std::uint8_t> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };

    if (!file.eof() && file.fail()) {
        error = "Could not read CHK file: " + input.string();
        return false;
    }

    return decode(
        data,
        map,
        error
    );
}

bool ChkDecoder::decode(
    const std::vector<std::uint8_t> &data,
    ChkMap &map,
    std::string &error
) const
{
    map = ChkMap{};

    if (data.empty()) {
        error = "CHK data is empty";
        return false;
    }

    static constexpr std::array<const char *, 8> TerrainNames = {
        "badlands",
        "platform",
        "install",
        "ashworld",
        "jungle",
        "desert",
        "arctic",
        "twilight"
    };

    std::size_t offset = 0;

    while (offset < data.size()) {
        if (!rangeFits(offset, 8, data.size())) {
            error =
                "CHK contains a truncated section header at offset " +
                std::to_string(offset);

            return false;
        }

        const std::string section = sectionName(
            data,
            offset
        );

        const std::uint32_t sectionLength = readLe32(
            data,
            offset + 4
        );

        const std::size_t payloadStart = offset + 8;

        if (!rangeFits(
            payloadStart,
            static_cast<std::size_t>(sectionLength),
            data.size()
        )) {
            error =
                "CHK section '" +
                section +
                "' exceeds input size";

            return false;
        }

        const std::size_t payloadEnd =
            payloadStart + static_cast<std::size_t>(sectionLength);

        if (section == "OWNR") {
            if (sectionLength == 12) {
                for (std::size_t player = 0; player < 12; ++player) {
                    int playerType = data[payloadStart + player];

                    if (
                        playerType != 0 &&
                        playerType != 3 &&
                        playerType != 5 &&
                        playerType != 6 &&
                        playerType != 7
                    ) {
                        playerType = 0;
                    }

                    map.playerType[player] = playerType;
                }
            }
        } else if (section == "ERA ") {
            if (sectionLength == 2) {
                const std::uint16_t terrain = readLe16(
                    data,
                    payloadStart
                );

                if (terrain >= TerrainNames.size()) {
                    error =
                        "CHK contains unsupported terrain index: " +
                        std::to_string(terrain);

                    return false;
                }

                map.terrain = TerrainNames[terrain];
            }
        } else if (section == "DIM ") {
            if (sectionLength == 4) {
                map.width = readLe16(
                    data,
                    payloadStart
                );

                map.height = readLe16(
                    data,
                    payloadStart + 2
                );
            }
        } else if (section == "SIDE") {
            if (sectionLength == 12) {
                for (std::size_t player = 0; player < 12; ++player) {
                    int race = data[payloadStart + player];

                    if (race == 5) {
                        race = 1;
                    }

                    if (
                        race > 2 &&
                        race != 4 &&
                        race != 7
                    ) {
                        race = 0;
                    }

                    map.playerRace[player] = race;
                }
            }
        } else if (section == "MTXM") {
            if (
                map.width > 0 &&
                map.height > 0
            ) {
                const std::size_t tileCount =
                    static_cast<std::size_t>(map.width) *
                    static_cast<std::size_t>(map.height);

                if (
                    tileCount <=
                    std::numeric_limits<std::size_t>::max() / 2
                ) {
                    const std::size_t expectedLength =
                        tileCount * 2;

                    if (
                        static_cast<std::size_t>(sectionLength) ==
                        expectedLength
                    ) {
                        map.tiles.resize(tileCount);

                        for (
                            std::size_t tile = 0;
                            tile < tileCount;
                            ++tile
                        ) {
                            map.tiles[tile] = readLe16(
                                data,
                                payloadStart + tile * 2
                            );
                        }
                    }
                }
            }
        } else if (section == "UNIT") {
            if (sectionLength % 36 == 0) {
                for (
                    std::size_t record = payloadStart;
                    record < payloadEnd;
                    record += 36
                ) {
                    ChkUnit unit;

                    unit.x = static_cast<std::uint16_t>(
                        readLe16(data, record + 4) / 32
                    );

                    unit.y = static_cast<std::uint16_t>(
                        readLe16(data, record + 6) / 32
                    );

                    unit.type = readLe16(
                        data,
                        record + 8
                    );

                    unit.properties = readLe16(
                        data,
                        record + 12
                    );

                    unit.validElements = readLe16(
                        data,
                        record + 14
                    );

                    unit.player = data[record + 16];
                    unit.hitPointsPercent = data[record + 17];
                    unit.shieldPointsPercent = data[record + 18];
                    unit.energyPointsPercent = data[record + 19];

                    unit.resourceAmount = readLe32(
                        data,
                        record + 20
                    );

                    unit.numUnitsIn = readLe16(
                        data,
                        record + 24
                    );

                    unit.stateFlags = static_cast<std::uint8_t>(
                        readLe16(data, record + 26)
                    );

                    if (unit.player == 11) {
                        unit.player =
                            static_cast<std::uint8_t>(
                                ChkPlayerCount - 1
                            );
                    }

                    if (unit.type == ChkStartLocationUnit) {
                        if (unit.player >= ChkPlayerCount) {
                            error =
                                "CHK start location references invalid player " +
                                std::to_string(unit.player);

                            return false;
                        }

                        map.playerStart[unit.player].x = unit.x;
                        map.playerStart[unit.player].y = unit.y;
                    } else {
                        map.units.push_back(unit);
                    }
                }
            }
        } else if (section == "STR ") {
            if (sectionLength >= 2) {
                const std::uint16_t stringCount = readLe16(
                    data,
                    payloadStart
                );

                const std::size_t offsetsLength =
                    static_cast<std::size_t>(stringCount) * 2;

                if (!rangeFits(
                    payloadStart + 2,
                    offsetsLength,
                    payloadEnd
                )) {
                    error =
                        "CHK STR section contains a truncated string offset table";

                    return false;
                }

                for (
                    std::size_t index = 0;
                    index < stringCount;
                    ++index
                ) {
                    const std::uint16_t stringOffset = readLe16(
                        data,
                        payloadStart + 2 + index * 2
                    );

                    bool valid = false;

                    std::string value = readChkString(
                        data,
                        payloadStart,
                        payloadEnd,
                        stringOffset,
                        valid
                    );

                    if (!valid) {
                        error =
                            "CHK STR section contains invalid string offset " +
                            std::to_string(stringOffset);

                        return false;
                    }

                    map.strings.push_back(
                        std::move(value)
                    );
                }

                map.description = "none";
            }
        } else if (section == "MRGN") {
            if (sectionLength % 20 == 0) {
                for (
                    std::size_t record = payloadStart;
                    record < payloadEnd;
                    record += 20
                ) {
                    ChkLocation location;

                    location.startX = readLe32(
                        data,
                        record
                    );

                    location.startY = readLe32(
                        data,
                        record + 4
                    );

                    location.endX = readLe32(
                        data,
                        record + 8
                    );

                    location.endY = readLe32(
                        data,
                        record + 12
                    );

                    location.stringNumber = readLe16(
                        data,
                        record + 16
                    );

                    location.flags = readLe16(
                        data,
                        record + 18
                    );

                    map.locations.push_back(location);
                }
            }
        } else if (section == "TRIG") {
            if (sectionLength % 2400 == 0) {
                for (
                    std::size_t record = payloadStart;
                    record < payloadEnd;
                    record += 2400
                ) {
                    ChkTrigger trigger;

                    std::size_t triggerOffset = record;

                    for (
                        ChkTriggerCondition &condition :
                        trigger.conditions
                    ) {
                        condition.location = readLe32(
                            data,
                            triggerOffset
                        );

                        condition.group = readLe32(
                            data,
                            triggerOffset + 4
                        );

                        condition.qualifiedNumber = readLe32(
                            data,
                            triggerOffset + 8
                        );

                        condition.unitType = readLe16(
                            data,
                            triggerOffset + 12
                        );

                        condition.compType = data[
                            triggerOffset + 14
                        ];

                        condition.condition = data[
                            triggerOffset + 15
                        ];

                        condition.resType = data[
                            triggerOffset + 16
                        ];

                        condition.flags = data[
                            triggerOffset + 17
                        ];

                        triggerOffset += 20;
                    }

                    for (
                        ChkTriggerAction &action :
                        trigger.actions
                    ) {
                        const std::uint32_t source = readLe32(data, triggerOffset);
                        action.source = source == 0 ? ChkNoLocation : source - static_cast<std::uint32_t>(1);

                        action.triggerNumber = readLe32(
                            data,
                            triggerOffset + 4
                        );

                        action.wavNumber = readLe32(
                            data,
                            triggerOffset + 8
                        );

                        action.time = readLe32(
                            data,
                            triggerOffset + 12
                        );

                        action.firstGroup = readLe32(
                            data,
                            triggerOffset + 16
                        );

                        action.secondGroup = readLe32(
                            data,
                            triggerOffset + 20
                        );

                        action.status = readLe16(
                            data,
                            triggerOffset + 24
                        );

                        action.action = data[
                            triggerOffset + 26
                        ];

                        action.numUnits = data[
                            triggerOffset + 27
                        ];

                        action.actionFlags = data[
                            triggerOffset + 28
                        ];

                        triggerOffset += 32;
                    }

                    map.triggers.push_back(
                        std::move(trigger)
                    );
                }
            }
        }

        offset = payloadEnd;
    }

    if (map.width <= 0 || map.height <= 0) {
        error = "CHK does not contain valid map dimensions";
        return false;
    }

    if (map.terrain.empty()) {
        error = "CHK does not contain a valid terrain section";
        return false;
    }

    const std::size_t expectedTiles =
        static_cast<std::size_t>(map.width) *
        static_cast<std::size_t>(map.height);

    if (map.tiles.size() != expectedTiles) {
        error =
            "CHK does not contain a complete MTXM tile map";
        return false;
    }

    return true;
}
