// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef UNIT_DEFINITION_H
#define UNIT_DEFINITION_H

#include <string>

struct UnitDefinition
{
    int id = -1;
    std::string name;
    bool extractor = true;
};

#endif // UNIT_DEFINITION_H
