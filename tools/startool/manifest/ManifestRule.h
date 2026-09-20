// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MANIFEST_RULE_H
#define MANIFEST_RULE_H

#include <string>

enum class ManifestRuleKind
{
    Exact
};

enum class ManifestOperation
{
    WavToOgg
};

struct ManifestRule
{
    std::string id;

    ManifestRuleKind kind = ManifestRuleKind::Exact;

    std::string source;
    std::string input;

    ManifestOperation operation = ManifestOperation::WavToOgg;

    std::string output;
};

#endif // MANIFEST_RULE_H