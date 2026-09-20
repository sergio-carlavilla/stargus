// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SOURCE_READER_REGISTRY_H
#define SOURCE_READER_REGISTRY_H

#include "SourceReader.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class SourceReaderRegistry
{
    public:
        bool add(
            std::string id,
            std::unique_ptr<SourceReader> reader
        );

        SourceReader *find(std::string_view id);

        const SourceReader *find(std::string_view id) const;

    private:
        std::unordered_map<std::string, std::unique_ptr<SourceReader>> readers_;
};

#endif // SOURCE_READER_REGISTRY_H
