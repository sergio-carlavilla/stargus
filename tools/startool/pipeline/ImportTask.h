// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef IMPORT_TASK_H
#define IMPORT_TASK_H

#include "manifest/ManifestRule.h"

#include <string>

struct ImportTask
{
    std::string id;
    std::string source;
    std::string input;

    ManifestOperation operation = ManifestOperation::WavToOgg;

    std::string palette;
    bool rgba = false;

    std::string output;
};

#endif // IMPORT_TASK_H
