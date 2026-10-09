// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef CASCARCHIVE_H
#define CASCARCHIVE_H

#include <CascLib.h>

#include <filesystem>
#include <string_view>

class CascArchive
{
    public:
        CascArchive() = default;

        explicit CascArchive(
            const std::filesystem::path &path
        );

        ~CascArchive();

        CascArchive(const CascArchive &) = delete;
        CascArchive &operator=(const CascArchive &) = delete;

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

#endif // CASCARCHIVE_H
