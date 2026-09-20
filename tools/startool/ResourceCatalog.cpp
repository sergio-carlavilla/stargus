// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ResourceCatalog.h"

#include <algorithm>
#include <cctype>
#include <utility>

void ResourceCatalog::add(Resource resource)
{
    resource.path = normalizePath(resource.path);

    const auto existing = index_.find(resource.path);

    if (existing != index_.end()) {
        resources_[existing->second] = std::move(resource);
        return;
    }

    const std::size_t index = resources_.size();
    index_[resource.path] = index;

    resources_.push_back(
        std::move(resource)
    );
}

bool ResourceCatalog::contains(std::string_view path) const
{
    return find(path) != nullptr;
}

const Resource *ResourceCatalog::find(std::string_view path) const
{
    const std::string normalized = normalizePath(path);
    const auto iterator = index_.find(normalized);

    if (iterator == index_.end()) {
        return nullptr;
    }

    return &resources_[iterator->second];
}

std::size_t ResourceCatalog::size() const
{
    return resources_.size();
}

const std::vector<Resource> & ResourceCatalog::resources() const
{
    return resources_;
}

std::string ResourceCatalog::normalizePath(std::string_view path)
{
    std::string normalized(path);

    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char character) {
        if (character == '\\') {
            return '/';
        }

        return static_cast<char>(
            std::tolower(character)
        );
    });

    return normalized;
}
