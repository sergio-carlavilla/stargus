// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef MPQVERIFIER_H
#define MPQVERIFIER_H

#include "GameSource.h"

class MpqVerifier
{
    public:
        bool verify(GameSource &source) const;
};

#endif // MPQVERIFIER_H
