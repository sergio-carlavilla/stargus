// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "LogicalSourceVerifier.h"

#include "SourceReaderFactory.h"

bool LogicalSourceVerifier::verify(const LogicalSource &source) const
{
    SourceReaderFactory factory;

    auto reader = factory.create(source);

    if (!reader) {
        return false;
    }

    return reader->isOpen();
}
