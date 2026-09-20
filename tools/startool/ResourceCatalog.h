// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef RESOURCECATALOG_H
#define RESOURCECATALOG_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct Resource
{
    std::string path;

    std::string nativePath;

    std::uint64_t size = 0;
};

class ResourceCatalog
{
    public:
        void add(Resource resource);

        bool contains(std::string_view path) const;

        const Resource *find(std::string_view path) const;

        std::size_t size() const;

        const std::vector<Resource> &resources() const;

        static std::string normalizePath(std::string_view path);

    private:
        std::vector<Resource> resources_;

        std::unordered_map<std::string, std::size_t> index_;
};

#endif // RESOURCECATALOG_H
