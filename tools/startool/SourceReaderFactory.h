// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SOURCE_READER_FACTORY_H
#define SOURCE_READER_FACTORY_H

#include "LogicalSource.h"
#include "SourceReader.h"

#include <memory>

class SourceReaderFactory
{
    public:
        std::unique_ptr<SourceReader> create(const LogicalSource &source) const;
};

#endif // SOURCE_READER_FACTORY_H
