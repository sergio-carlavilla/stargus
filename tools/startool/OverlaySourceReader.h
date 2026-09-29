// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef OVERLAY_SOURCE_READER_H
#define OVERLAY_SOURCE_READER_H

#include "SourceReader.h"

#include <filesystem>
#include <memory>
#include <string_view>

class OverlaySourceReader : public SourceReader
{
    public:
        OverlaySourceReader(
            std::unique_ptr<SourceReader> primary,
            std::unique_ptr<SourceReader> fallback
        );

        bool isOpen() const override;

        bool contains(std::string_view resource) const override;

        bool extract(
            std::string_view resource,
            const std::filesystem::path &destination
        ) const override;

    private:
        std::unique_ptr<SourceReader> primary_;
        std::unique_ptr<SourceReader> fallback_;
};

#endif // OVERLAY_SOURCE_READER_H
