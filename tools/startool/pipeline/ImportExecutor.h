// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#ifndef IMPORT_EXECUTOR_H
#define IMPORT_EXECUTOR_H

#include "ImportTask.h"
#include "SourceReaderRegistry.h"
#include "palette/Palette.h"

#include <filesystem>
#include <map>
#include <string>

class ImportExecutor
{

    public:
        bool stage(
            const ImportTask &task,
            const SourceReaderRegistry &readers,
            const std::filesystem::path &stagingRoot,
            std::filesystem::path &stagedPath,
            std::string &error
        ) const;

        bool execute(
            const ImportTask &task,
            const SourceReaderRegistry &readers,
            const std::map<std::string, Palette> &palettes,
            const std::filesystem::path &destinationRoot,
            std::filesystem::path &outputPath,
            std::string &error
        ) const;

};

#endif // IMPORT_EXECUTOR_H
