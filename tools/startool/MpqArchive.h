// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef MPQARCHIVE_H
#define MPQARCHIVE_H

#include <StormLib.h>

#include <filesystem>
#include <string_view>

class MpqArchive
{
    public:
        MpqArchive() = default;

        explicit MpqArchive(
            const std::filesystem::path &path
        );

        ~MpqArchive();

        MpqArchive(const MpqArchive &) = delete;
        MpqArchive &operator=(const MpqArchive &) = delete;

        bool open(
            const std::filesystem::path &path
        );

        void close();

        bool isOpen() const;

        bool contains(
            std::string_view member
        ) const;

        bool extract(
            std::string_view member,
            const std::filesystem::path &destination
        ) const;

    private:
        HANDLE handle_ = nullptr;
};

#endif // MPQARCHIVE_H
