// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MANIFEST_H
#define MANIFEST_H

#include "ManifestRule.h"

#include <vector>

struct Manifest
{
    int formatVersion = 0;
    std::vector<ManifestRule> rules;
};

#endif // MANIFEST_H