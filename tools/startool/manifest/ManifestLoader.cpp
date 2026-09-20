// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "ManifestLoader.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <set>
#include <string>
#include <filesystem>
#include <string_view>

namespace
{

    bool isValidManifestPath(std::string_view value)
    {
        if (value.empty()) {
            return false;
        }

        if (value.find('\\') != std::string_view::npos) {
            return false;
        }

        const std::filesystem::path path(value);
        if (path.is_absolute()) {
            return false;
        }

        for (const auto &component : path) {
            if (component == "." || component == "..") {
                return false;
            }
        }

        return true;
    }

    bool hasOnlyFields(
        const nlohmann::json &object,
        const std::set<std::string> &allowedFields,
        std::string &unknownField
    )
    {
        for (const auto &item : object.items()) {
            if (allowedFields.find(item.key()) == allowedFields.end()) {
                unknownField = item.key();
                return false;
            }
        }

        return true;
    }

}

std::optional<Manifest> ManifestLoader::load(
    const std::filesystem::path &path,
    std::string &error
) const
{
    std::ifstream input(path);

    if (!input) {
        error = "Could not open manifest: " + path.string();
        return std::nullopt;
    }

    nlohmann::json document;

    try {
        input >> document;
    } catch (const nlohmann::json::parse_error &exception) {
        error = "Invalid JSON: " + std::string(exception.what());
        return std::nullopt;
    }

    if (!document.is_object()) {
        error = "Manifest root must be an object";
        return std::nullopt;
    }

    const std::set<std::string> allowedManifestFields = {
        "format_version",
        "rules"
    };

    std::string unknownField;

    if (!hasOnlyFields(document, allowedManifestFields, unknownField)) {
        error = "Unknown manifest field: " + unknownField;
        return std::nullopt;
    }

    if (!document.contains("format_version") || !document["format_version"].is_number_integer()) {
        error = "Manifest must contain an integer format_version";
        return std::nullopt;
    }

    Manifest manifest;
    manifest.formatVersion = document["format_version"].get<int>();

    if (manifest.formatVersion != 1) {
        error = "Unsupported manifest format version: " + std::to_string(manifest.formatVersion);
        return std::nullopt;
    }

    if (!document.contains("rules") || !document["rules"].is_array()) {
        error = "Manifest must contain a rules array";
        return std::nullopt;
    }

    std::set<std::string> ruleIds;
    for (const auto &ruleDocument : document["rules"]) {
        if (!ruleDocument.is_object()) {
            error = "Every manifest rule must be an object";
            return std::nullopt;
        }

        const char *requiredFields[] = {
            "id",
            "kind",
            "source",
            "input",
            "operation",
            "output"
        };

        for (const char *field : requiredFields) {
            if (!ruleDocument.contains(field) || !ruleDocument[field].is_string()) {
                error = "Manifest rule is missing string field: " + std::string(field);
                return std::nullopt;
            }
        }

        ManifestRule rule;
        rule.id = ruleDocument["id"].get<std::string>();

        const std::set<std::string> allowedRuleFields = {
            "id",
            "kind",
            "source",
            "input",
            "operation",
            "output"
        };

        if (!hasOnlyFields(ruleDocument, allowedRuleFields, unknownField)) {
            error = "Manifest rule '" + rule.id + "' contains unknown field: " + unknownField;
            return std::nullopt;
        }

        const std::string kind = ruleDocument["kind"].get<std::string>();

        if (kind != "exact") {
            error = "Unsupported manifest rule kind: " + kind;
            return std::nullopt;
        }

        rule.kind = ManifestRuleKind::Exact;
        rule.source = ruleDocument["source"].get<std::string>();
        rule.input = ruleDocument["input"].get<std::string>();

        const std::string operation = ruleDocument["operation"].get<std::string>();

        if (operation == "extract") {
            rule.operation = ManifestOperation::Extract;
        } else if (operation == "pcx_to_png") {
            rule.operation = ManifestOperation::PcxToPng;
        } else if (operation == "wav_to_ogg") {
            rule.operation = ManifestOperation::WavToOgg;
        } else {
            error = "Unsupported manifest operation: " + operation;
            return std::nullopt;
        }

        rule.output = ruleDocument["output"].get<std::string>();

        if (rule.id.empty()) {
            error = "Manifest rule id cannot be empty";
            return std::nullopt;
        }

        if (rule.source.empty()) {
            error = "Manifest rule '" + rule.id + "' has an empty source";
            return std::nullopt;
        }

        if (!isValidManifestPath(rule.input)) {
            error = "Manifest rule '" + rule.id + "' has an invalid input path: " + rule.input;
            return std::nullopt;
        }

        if (!isValidManifestPath(rule.output)) {
            error = "Manifest rule '" + rule.id + "' has an invalid output path: " + rule.output;
            return std::nullopt;
        }

        if (!ruleIds.insert(rule.id).second) {
            error = "Duplicate manifest rule id: " + rule.id;
            return std::nullopt;
        }

        manifest.rules.push_back(
            std::move(rule)
        );
    }

    return manifest;
}
