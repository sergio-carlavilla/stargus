// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SourceReaderRegistry.h"

#include <utility>

bool SourceReaderRegistry::add(
    std::string id,
    std::unique_ptr<SourceReader> reader
)
{
    if (!reader) {
        return false;
    }

    const auto result = readers_.emplace(std::move(id), std::move(reader));
    return result.second;
}

SourceReader *SourceReaderRegistry::find(std::string_view id)
{
    const auto iterator = readers_.find(std::string(id));
    if (iterator == readers_.end()) {
        return nullptr;
    }

    return iterator->second.get();
}

const SourceReader *SourceReaderRegistry::find(std::string_view id) const
{
    const auto iterator = readers_.find(std::string(id));
    if (iterator == readers_.end()) {
        return nullptr;
    }

    return iterator->second.get();
}
