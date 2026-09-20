// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef LOGICALSOURCEVERIFIER_H
#define LOGICALSOURCEVERIFIER_H

#include "LogicalSource.h"

class LogicalSourceVerifier
{
    public:
        bool verify(const LogicalSource &source) const;
};

#endif // LOGICALSOURCEVERIFIER_H
