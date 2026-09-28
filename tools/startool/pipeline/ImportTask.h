// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef IMPORT_TASK_H
#define IMPORT_TASK_H

#include "manifest/ManifestRule.h"
#include "manifest/UnitDefinition.h"

#include <string>
#include <vector>

struct ImportTask
{
    std::string id;
    std::string source;
    std::string input;

    ManifestOperation operation = ManifestOperation::WavToOgg;

    std::string palette;
    bool rgba = false;
    int frame = -1;
    std::vector<int> frames;

    std::string table;
    int image = -1;

    int left = -1;
    int right = -1;
    int width = -1;
    int height = -1;

    int unit = -1;
    std::string ident;

    std::vector<UnitDefinition> units;

    std::string output;
};

#endif // IMPORT_TASK_H
