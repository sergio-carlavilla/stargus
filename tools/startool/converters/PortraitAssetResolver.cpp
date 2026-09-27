// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "PortraitAssetResolver.h"

#include "formats/PortDataDecoder.h"
#include "formats/TblDecoder.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <utility>
#include <vector>

namespace
{
    std::string normalizePath(std::string value)
    {
        std::replace(value.begin(), value.end(), '\\', '/');
        return value;
    }

    std::string toLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            }
        );

        return value;
    }

    std::string portraitId(const std::string &value)
    {
        const std::filesystem::path path(normalizePath(value));
        return toLower(path.parent_path().generic_string());
    }
}

bool PortraitAssetResolver::resolve(
    const std::filesystem::path &portdataDat,
    const std::filesystem::path &portdataTbl,
    std::vector<PortraitAssetMetadata> &portraits,
    std::string &error
) const
{
    PortDataDecoder portdataDecoder;
    TblDecoder tblDecoder;

    std::vector<PortDataRecord> records;
    std::vector<TblEntry> table;

    if (!portdataDecoder.decode(portdataDat, records, error)) {
        return false;
    }

    if (!tblDecoder.decode(portdataTbl, table, error)) {
        return false;
    }

    portraits.clear();
    portraits.reserve(records.size());

    for (std::size_t index = 0; index < records.size(); ++index) {
        const PortDataRecord &record = records[index];

        if (record.videoIdle == 0) {
            error = "Portrait " + std::to_string(index) + " has a null idle portdata.tbl reference";
            return false;
        }

        if (record.videoTalking == 0) {
            error = "Portrait " + std::to_string(index) + " has a null talking portdata.tbl reference";
            return false;
        }

        const std::size_t idleTableIndex = static_cast<std::size_t>(record.videoIdle - 1);
        const std::size_t talkingTableIndex = static_cast<std::size_t>(record.videoTalking - 1);

        if (idleTableIndex >= table.size()) {
            error = "Portrait " + std::to_string(index) + " references invalid idle portdata.tbl entry " + std::to_string(idleTableIndex);
            return false;
        }

        if (talkingTableIndex >= table.size()) {
            error = "Portrait " + std::to_string(index) + " references invalid talking portdata.tbl entry " + std::to_string(talkingTableIndex);
            return false;
        }

        const std::string idle = normalizePath(table[idleTableIndex].name);
        const std::string talking = normalizePath(table[talkingTableIndex].name);
        const std::string idleId = portraitId(idle);
        const std::string talkingId = portraitId(talking);

        if (idleId != talkingId) {
            error = "Portrait " + std::to_string(index) + " has inconsistent idle/talking ids: " + idleId + " != " + talkingId;
            return false;
        }

        PortraitAssetMetadata metadata;
        metadata.index = index;
        metadata.id = idleId;
        metadata.idleInputBase = idle;
        metadata.talkingInputBase = talking;
        metadata.idleOutputBase = toLower(idle);
        metadata.talkingOutputBase = toLower(talking);

        portraits.push_back(std::move(metadata));
    }

    return true;
}
