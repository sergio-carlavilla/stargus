// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef FILESYSTEM_SOURCE_READER_H
#define FILESYSTEM_SOURCE_READER_H

#include "LogicalSource.h"
#include "SourceReader.h"

#include <filesystem>
#include <optional>
#include <string_view>

// Reads loose installation files without accessing CASC or MPQ archives
// Requested paths must stay inside the configured source directory
class FilesystemSourceReader final : public SourceReader
{
    public:
        explicit FilesystemSourceReader(const LogicalSource &source);

        bool isOpen() const override;
        bool contains(std::string_view resource) const override;
        bool extract(
            std::string_view resource,
            const std::filesystem::path &destination
        ) const override;

    private:
        std::optional<std::filesystem::path> resolve(
            std::string_view resource
        ) const;

        std::filesystem::path root_;
        bool open_ = false;
};

#endif // FILESYSTEM_SOURCE_READER_H
