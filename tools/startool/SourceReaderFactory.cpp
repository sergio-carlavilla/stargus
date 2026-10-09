// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "SourceReaderFactory.h"
#include "MpqSourceReader.h"
#include "FilesystemSourceReader.h"

#ifdef HAVE_CASC
#include "CascSourceReader.h"
#endif

#include <memory>

std::unique_ptr<SourceReader> SourceReaderFactory::create(const LogicalSource &source) const
{
    switch (source.format) {
        case SourceFormat::Directory:
            return std::make_unique<FilesystemSourceReader>(source);

        case SourceFormat::Mpq:
            return std::make_unique<MpqSourceReader>(source);

#ifdef HAVE_CASC
        case SourceFormat::Casc:
            return std::make_unique<CascSourceReader>(source);
#endif

        default:
            return nullptr;
    }
}
