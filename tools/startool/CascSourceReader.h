// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef CASCSOURCEREADER_H
#define CASCSOURCEREADER_H

#include "CascArchive.h"
#include "LogicalSource.h"
#include "SourceReader.h"

class CascSourceReader final : public SourceReader
{
    public:
        explicit CascSourceReader(const LogicalSource &source);
        ~CascSourceReader() override = default;

        bool isOpen() const override;

        bool contains(std::string_view resource) const override;

        bool extract(
            std::string_view resource,
            const std::filesystem::path &destination
        ) const override;

    private:
        CascArchive archive_;
        GameLocale locale_ = GameLocale::EnUS;
};

#endif // CASCSOURCEREADER_H
