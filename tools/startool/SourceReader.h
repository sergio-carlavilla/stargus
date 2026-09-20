// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef SOURCEREADER_H
#define SOURCEREADER_H

#include <filesystem>
#include <string_view>

class SourceReader
{
    public:
        virtual ~SourceReader() = default;

        virtual bool isOpen() const = 0;

        virtual bool contains(std::string_view resource) const = 0;

        virtual bool extract(
            std::string_view resource,
            const std::filesystem::path &destination
        ) const = 0;
};

#endif // SOURCEREADER_H
