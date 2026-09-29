// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifndef STARGUS_SOURCE_DIR
#error "STARGUS_SOURCE_DIR must be defined"
#endif

namespace
{
    using Json = nlohmann::json;

    struct ManifestStats
    {
        std::size_t ruleCount = 0;
        std::size_t outputCount = 0;

        std::set<std::string> palettes;
        std::set<std::string> rules;
        std::set<std::string> outputs;
        std::set<std::filesystem::path> visited;

        std::map<std::string, std::vector<std::string>>
            paletteReferences;

        std::vector<std::string> errors;
    };

    bool loadJson(
        const std::filesystem::path &path,
        Json &json,
        std::string &error
    )
    {
        std::ifstream input(path);

        if (!input) {
            error =
                "Could not open manifest: " +
                path.string();

            return false;
        }

        try {
            input >> json;
        } catch (const std::exception &exception) {
            error =
                "Could not parse manifest '" +
                path.string() +
                "': " +
                exception.what();

            return false;
        }

        return true;
    }

    void addUnique(
        std::set<std::string> &values,
        const std::string &value,
        const std::string &kind,
        const std::filesystem::path &manifest,
        ManifestStats &stats
    )
    {
        if (!values.insert(value).second) {
            stats.errors.push_back(
                "Duplicate " +
                kind +
                " '" +
                value +
                "' in " +
                manifest.string()
            );
        }
    }

    bool collectManifest(
        const std::filesystem::path &manifestRoot,
        const std::filesystem::path &manifest,
        ManifestStats &stats,
        std::string &error
    )
    {
        std::error_code filesystemError;

        const std::filesystem::path normalized =
            std::filesystem::weakly_canonical(
                manifest,
                filesystemError
            );

        if (filesystemError) {
            error =
                "Could not resolve manifest path '" +
                manifest.string() +
                "': " +
                filesystemError.message();

            return false;
        }

        if (!stats.visited.insert(normalized).second) {
            return true;
        }

        Json json;

        if (!loadJson(
            normalized,
            json,
            error
        )) {
            return false;
        }

        if (json.contains("palettes")) {
            for (const Json &palette : json["palettes"]) {
                const std::string id =
                    palette.value("id", "");

                if (id.empty()) {
                    stats.errors.push_back(
                        "Palette without id in " +
                        normalized.string()
                    );

                    continue;
                }

                addUnique(
                    stats.palettes,
                    id,
                    "palette id",
                    normalized,
                    stats
                );
            }
        }

        if (json.contains("rules")) {
            for (const Json &rule : json["rules"]) {
                ++stats.ruleCount;

                const std::string id =
                    rule.value("id", "");

                if (id.empty()) {
                    stats.errors.push_back(
                        "Rule without id in " +
                        normalized.string()
                    );
                } else {
                    addUnique(
                        stats.rules,
                        id,
                        "rule id",
                        normalized,
                        stats
                    );
                }

                if (
                    rule.contains("output") &&
                    rule["output"].is_string()
                ) {
                    const std::string output =
                        rule["output"].get<std::string>();

                    if (!output.empty()) {
                        ++stats.outputCount;

                        addUnique(
                            stats.outputs,
                            output,
                            "output",
                            normalized,
                            stats
                        );
                    }
                }

                if (
                    rule.contains("palette") &&
                    rule["palette"].is_string()
                ) {
                    const std::string palette =
                        rule["palette"].get<std::string>();

                    if (!palette.empty()) {
                        stats.paletteReferences[palette]
                            .push_back(id);
                    }
                }
            }
        }

        if (json.contains("includes")) {
            for (const Json &include : json["includes"]) {
                if (!include.is_string()) {
                    stats.errors.push_back(
                        "Non-string include in " +
                        normalized.string()
                    );

                    continue;
                }

                const std::string relative =
                    include.get<std::string>();

                if (
                    relative.find("test") !=
                    std::string::npos
                ) {
                    stats.errors.push_back(
                        "Production manifest includes test data: " +
                        relative
                    );
                }

                if (!collectManifest(
                    manifestRoot,
                    manifestRoot / relative,
                    stats,
                    error
                )) {
                    return false;
                }
            }
        }

        return true;
    }

    bool validateRoot(
        const std::filesystem::path &manifestRoot,
        const std::string &rootName,
        std::size_t expectedPalettes,
        std::size_t expectedRules,
        std::size_t expectedOutputs
    )
    {
        ManifestStats stats;
        std::string error;

        if (!collectManifest(
            manifestRoot,
            manifestRoot / rootName,
            stats,
            error
        )) {
            std::cerr
                << rootName
                << ": "
                << error
                << '\n';

            return false;
        }

        for (
            const auto &[palette, rules] :
            stats.paletteReferences
        ) {
            if (
                stats.palettes.find(palette) ==
                stats.palettes.end()
            ) {
                stats.errors.push_back(
                    "Unknown palette '" +
                    palette +
                    "' referenced by " +
                    std::to_string(rules.size()) +
                    " rule(s)"
                );
            }
        }

        const std::set<std::string> dynamicPalettes{
            "ofire",
            "gfire",
            "bfire",
            "bexpl"
        };

        for (
            const std::string &palette :
            dynamicPalettes
        ) {
            if (
                stats.palettes.find(palette) ==
                stats.palettes.end()
            ) {
                stats.errors.push_back(
                    "Missing dynamic image palette '" +
                    palette +
                    "'"
                );
            }
        }

        if (stats.palettes.size() != expectedPalettes) {
            stats.errors.push_back(
                "Expected " +
                std::to_string(expectedPalettes) +
                " palettes, found " +
                std::to_string(stats.palettes.size())
            );
        }

        if (stats.ruleCount != expectedRules) {
            stats.errors.push_back(
                "Expected " +
                std::to_string(expectedRules) +
                " rules, found " +
                std::to_string(stats.ruleCount)
            );
        }

        if (stats.rules.size() != expectedRules) {
            stats.errors.push_back(
                "Unique rule count differs from expected rule count"
            );
        }

        if (stats.outputCount != expectedOutputs) {
            stats.errors.push_back(
                "Expected " +
                std::to_string(expectedOutputs) +
                " outputs, found " +
                std::to_string(stats.outputCount)
            );
        }

        if (stats.outputs.size() != expectedOutputs) {
            stats.errors.push_back(
                "Unique output count differs from expected output count"
            );
        }

        if (!stats.errors.empty()) {
            std::cerr
                << "Manifest regression validation failed for "
                << rootName
                << ":\n";

            for (const std::string &message : stats.errors) {
                std::cerr
                    << "  - "
                    << message
                    << '\n';
            }

            return false;
        }

        std::cout
            << rootName
            << ": palettes="
            << stats.palettes.size()
            << ", rules="
            << stats.ruleCount
            << ", outputs="
            << stats.outputCount
            << '\n';

        return true;
    }
}

int main()
{
    const std::filesystem::path manifestRoot =
        std::filesystem::path(STARGUS_SOURCE_DIR) /
        "manifests";

    const bool classicOk =
        validateRoot(
            manifestRoot,
            "classic.json",
            19,
            2764,
            2166
        );

    const bool broodWarOk =
        validateRoot(
            manifestRoot,
            "broodwar.json",
            22,
            3898,
            3132
        );

    return classicOk && broodWarOk
        ? 0
        : 1;
}
