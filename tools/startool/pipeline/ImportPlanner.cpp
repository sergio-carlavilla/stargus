// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ImportPlanner.h"

#include <utility>

std::vector<ImportTask> ImportPlanner::plan(const Manifest &manifest) const
{
    std::vector<ImportTask> tasks;

    for (const ManifestRule &rule : manifest.rules) {
        switch (rule.kind) {
            case ManifestRuleKind::Exact: {
                ImportTask task;

                task.id = rule.id;
                task.source = rule.source;
                task.input = rule.input;
                task.operation = rule.operation;
                task.palette = rule.palette;
                task.rgba = rule.rgba;
                task.output = rule.output;

                tasks.push_back(
                    std::move(task)
                );

                break;
            }
        }
    }

    return tasks;
}
