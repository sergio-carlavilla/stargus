// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef CASCVERIFIER_H
#define CASCVERIFIER_H

#include "SourceDetector.h"

class CascVerifier
{
    public:
        bool verify(GameSource &source) const;
};

#endif // CASCVERIFIER_H
