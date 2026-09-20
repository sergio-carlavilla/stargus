// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef IMPORT_PLANNER_H
#define IMPORT_PLANNER_H

#include "ImportTask.h"
#include "manifest/Manifest.h"

#include <vector>

class ImportPlanner
{

    public:
        std::vector<ImportTask> plan(const Manifest &manifest) const;

};

#endif // IMPORT_PLANNER_H
