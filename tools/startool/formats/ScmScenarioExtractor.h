// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SCM_SCENARIO_EXTRACTOR_H
#define SCM_SCENARIO_EXTRACTOR_H

#include <filesystem>
#include <string>

class ScmScenarioExtractor
{

    public:
        bool extract(
            const std::filesystem::path &scm,
            const std::filesystem::path &output,
            std::string &error
        ) const;

};

#endif // SCM_SCENARIO_EXTRACTOR_H
