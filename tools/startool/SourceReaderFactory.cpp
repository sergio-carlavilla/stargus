// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SourceReaderFactory.h"
#include "MpqSourceReader.h"

#include <memory>

std::unique_ptr<SourceReader>SourceReaderFactory::create(const LogicalSource &source) const
{
    switch (source.format) {
        case SourceFormat::Mpq:
            return std::make_unique<MpqSourceReader>(
                source
            );

        default:
            return nullptr;
    }
}
