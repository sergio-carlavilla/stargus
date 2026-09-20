// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "LogicalSourceVerifier.h"
#include "MpqSourceReader.h"

bool LogicalSourceVerifier::verify(const LogicalSource &source) const
{
    if (source.format != SourceFormat::Mpq) {
        return false;
    }

    MpqSourceReader reader(source);
    return reader.isOpen();
}
