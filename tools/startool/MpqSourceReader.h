// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef MPQSOURCEREADER_H
#define MPQSOURCEREADER_H

#include "LogicalSource.h"
#include "SourceReader.h"
#include "MpqArchive.h"

#include <filesystem>
#include <string>
#include <string_view>

class MpqSourceReader : public SourceReader
{
    public:
        explicit MpqSourceReader(const LogicalSource &source);

        ~MpqSourceReader();

        MpqSourceReader(const MpqSourceReader &) = delete;
        MpqSourceReader &operator=(const MpqSourceReader &) = delete;

        bool isOpen() const;

        bool contains(std::string_view resource) const;

        bool extract(
            std::string_view resource,
            const std::filesystem::path &destination
        ) const;

    private:
        bool open(const LogicalSource &source);

        void cleanup();

        static std::string toMpqPath(std::string_view path);

        MpqArchive archive_;
        std::filesystem::path temporaryArchive_;
};

#endif // MPQARCHIVE_H
