// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "OverlaySourceReader.h"

#include <utility>

OverlaySourceReader::OverlaySourceReader(
    std::unique_ptr<SourceReader> primary,
    std::unique_ptr<SourceReader> fallback
): primary_(std::move(primary)), fallback_(std::move(fallback)) { }

bool OverlaySourceReader::isOpen() const
{
    return
        primary_ != nullptr &&
        fallback_ != nullptr &&
        primary_->isOpen() &&
        fallback_->isOpen();
}

bool OverlaySourceReader::contains(std::string_view resource) const
{
    if (!isOpen()) {
        return false;
    }

    return primary_->contains(resource) || fallback_->contains(resource);
}

bool OverlaySourceReader::extract(std::string_view resource, const std::filesystem::path &destination) const
{
    if (!isOpen()) {
        return false;
    }

    // Expansion resources always win. If the primary source contains the
    // resource but extraction fails, do not silently fall back to the base
    // game and hide a damaged expansion archive.
    if (primary_->contains(resource)) {
        return primary_->extract(resource, destination);
    }

    if (fallback_->contains(resource)) {
        return fallback_->extract(resource, destination);
    }

    return false;
}
