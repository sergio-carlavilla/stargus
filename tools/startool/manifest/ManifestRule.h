// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MANIFEST_RULE_H
#define MANIFEST_RULE_H

#include <string>
#include <vector>

enum class ManifestRuleKind
{
    Exact
};

enum class ManifestOperation
{
    Extract,
    GrpFrameToPng,
    GrpFramesToPng,
    GrpToPng,
    ImageAssets,
    ChkToMap,
    PcxToPng,
    ScmToMap,
    TilesetToLua,
    TilesetToPng,
    TextToUtf8,
    UnitLua,
    UnitsLua,
    WavToOgg
};

struct ManifestRule
{
    std::string id;

    ManifestRuleKind kind = ManifestRuleKind::Exact;

    std::string source;
    std::string input;

    ManifestOperation operation = ManifestOperation::WavToOgg;

    std::string palette;
    bool rgba = false;
    int frame = -1;
    std::vector<int> frames;

    std::string table;
    int image = -1;

    int unit = -1;
    std::string ident;

    std::string output;
};

#endif // MANIFEST_RULE_H
