// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "UnitsDatDecoder.h"

#include "BinaryReader.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{

    constexpr std::size_t UnitCount = 228;
    constexpr std::size_t SoundUnitCount = 106;
    constexpr std::size_t BuildingCount = 96;

    constexpr std::size_t StarCraftFileSize = 19192;
    constexpr std::size_t BroodWarFileSize = 19876;

    void skip(
        std::size_t &offset,
        std::size_t count,
        std::size_t elementSize
    )
    {
        offset += count * elementSize;
    }

}

bool UnitsDatDecoder::decode(
    const std::filesystem::path &input,
    std::vector<UnitsDatRecord> &records,
    std::string &error
) const
{
    std::ifstream file(input, std::ios::binary);

    if (!file) {
        error = "Could not open units.dat file: " + input.string();
        return false;
    }

    const std::vector<unsigned char> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    };

    const bool isStarCraft = data.size() == StarCraftFileSize;
    const bool isBroodWar = data.size() == BroodWarFileSize;

    if (!isStarCraft && !isBroodWar) {
        error = "Unsupported units.dat size " + std::to_string(data.size()) + " bytes: " + input.string();
        return false;
    }

    records.clear();
    records.resize(UnitCount);

    std::size_t offset = 0;

    // flingy
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].flingy = data[offset + index];
    }

    skip(offset, UnitCount, 1);

    // subunit1
    skip(offset, UnitCount, 2);

    // subunit2
    skip(offset, UnitCount, 2);

    // infestation
    skip(offset, BuildingCount, 2);

    // construction_animation
    skip(offset, UnitCount, 4);

    // unit_direction
    skip(offset, UnitCount, 1);

    // shield_enable
    skip(offset, UnitCount, 1);

    // shield_amount
    skip(offset, UnitCount, 2);

    // hit_points
    for (std::size_t index = 0; index < UnitCount; ++index) {
        const std::size_t recordOffset = offset + index * 4;

        records[index].hitPoints =
            static_cast<std::uint32_t>(
                BinaryReader::readBe16(data, recordOffset)
            ) +
            static_cast<std::uint32_t>(
                BinaryReader::readBe16(data, recordOffset + 2)
            );
    }

    skip(offset, UnitCount, 4);

    // elevation_level
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].elevationLevel = data[offset + index];
    }

    skip(offset, UnitCount, 1);

    // unknown
    skip(offset, UnitCount, 1);

    // rank
    skip(offset, UnitCount, 1);

    // ai_computer_idle
    skip(offset, UnitCount, 1);

    // ai_human_idle
    skip(offset, UnitCount, 1);

    // ai_return_to_idle
    skip(offset, UnitCount, 1);

    // ai_attack_unit
    skip(offset, UnitCount, 1);

    // ai_attack_move
    skip(offset, UnitCount, 1);

    // ground_weapon
    skip(offset, UnitCount, 1);

    // max_ground_hits: only Brood War
    if (isBroodWar) {
        skip(offset, UnitCount, 1);
    }

    // air_weapon
    skip(offset, UnitCount, 1);

    // max_air_hits: only Brood War
    if (isBroodWar) {
        skip(offset, UnitCount, 1);
    }

    // ai_internal
    skip(offset, UnitCount, 1);

    // special_ability_flags
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].specialAbilityFlags = BinaryReader::readLe32(data, offset + index * 4);
    }

    skip(offset, UnitCount, 4);

    // target_acquisition_range
    skip(offset, UnitCount, 1);

    // sight_range
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].sightRange = data[offset + index];
    }

    skip(offset, UnitCount, 1);

    // armor_upgrade
    skip(offset, UnitCount, 1);

    // unit_size
    skip(offset, UnitCount, 1);

    // armor
    skip(offset, UnitCount, 1);

    // right_click_action
    skip(offset, UnitCount, 1);

    // ready_sound
    for (std::size_t index = 0; index < SoundUnitCount; ++index) {
        records[index].readySound = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, SoundUnitCount, 2);

    // what_sound_start
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].whatSoundStart = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, UnitCount, 2);

    // what_sound_end
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].whatSoundEnd = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, UnitCount, 2);

    // piss_sound_start
    for (std::size_t index = 0; index < SoundUnitCount; ++index) {
        records[index].pissSoundStart = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, SoundUnitCount, 2);

    // piss_sound_end
    for (std::size_t index = 0; index < SoundUnitCount; ++index) {
        records[index].pissSoundEnd = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, SoundUnitCount, 2);

    // yes_sound_start
    for (std::size_t index = 0; index < SoundUnitCount; ++index) {
        records[index].yesSoundStart = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, SoundUnitCount, 2);

    // yes_sound_end
    for (std::size_t index = 0; index < SoundUnitCount; ++index) {
        records[index].yesSoundEnd = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, SoundUnitCount, 2);

    // staredit_placement_box
    skip(offset, UnitCount, 4);

    // addon_position
    skip(offset, BuildingCount, 4);

    // unit_dimension
    for (std::size_t index = 0; index < UnitCount; ++index) {
        const std::size_t recordOffset = offset + index * 8;

        UnitDimensions &dimensions = records[index].dimensions;

        dimensions.left = BinaryReader::readLe16(data, recordOffset);
        dimensions.up = BinaryReader::readLe16(data, recordOffset + 2);
        dimensions.right = BinaryReader::readLe16(data, recordOffset + 4);
        dimensions.down = BinaryReader::readLe16(data, recordOffset + 6);
    }

    skip(offset, UnitCount, 8);

    // portrait
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].portrait = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, UnitCount, 2);

    // mineral_cost
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].mineralCost = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, UnitCount, 2);

    // vespene_cost
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].vespeneCost = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, UnitCount, 2);

    // build_time
    for (std::size_t index = 0; index < UnitCount; ++index) {
        records[index].buildTime = BinaryReader::readLe16(data, offset + index * 2);
    }

    skip(offset, UnitCount, 2);

    // requirements
    skip(offset, UnitCount, 2);

    // staredit_group_flags
    skip(offset, UnitCount, 1);

    // supply_provided
    skip(offset, UnitCount, 1);

    // supply_required
    skip(offset, UnitCount, 1);

    // space_required
    skip(offset, UnitCount, 1);

    // space_provided
    skip(offset, UnitCount, 1);

    // build_score
    skip(offset, UnitCount, 2);

    // destroy_score
    skip(offset, UnitCount, 2);

    // unit_map_string
    skip(offset, UnitCount, 2);

    // broodwar_flag: only Brood War
    if (isBroodWar) {
        skip(offset, UnitCount, 1);
    }

    // staredit_availability_flags
    skip(offset, UnitCount, 2);

    if (offset != data.size()) {
        error = "units.dat layout consumed " + std::to_string(offset) + " bytes but file contains " + std::to_string(data.size()) + " bytes";

        records.clear();
        return false;
    }

    return true;
}
