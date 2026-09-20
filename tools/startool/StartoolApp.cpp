// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "StartoolApp.h"

#include "LogicalSource.h"
#include "LogicalSourceVerifier.h"
#include "MpqSourceReader.h"
#include "MpqVerifier.h"
#include "SourceDetector.h"
#include "SourceReaderRegistry.h"
#include "SourceReaderFactory.h"
#include "manifest/ManifestLoader.h"
#include "manifest/ManifestRule.h"
#include "pipeline/ImportPlanner.h"
#include "pipeline/ImportTask.h"
#include "pipeline/ImportExecutor.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <memory>
#include <system_error>

int StartoolApp::run(int argc, char **argv)
{
    if (argc < 2) {
        printHelp();
        return 0;
    }

    const std::string command = argv[1];

    if (command == "help" || command == "--help" || command == "-h") {
        printHelp();
        return 0;
    }

    if (command == "version" || command == "--version") {
        printVersion();
        return 0;
    }

    if (command == "inspect") {
        return runInspect(argc, argv);
    }

    if (command == "extract") {
        return runExtract(argc, argv);
    }

    if (command == "import") {
        return runImport(argc, argv);
    }

    if (command == "verify") {
        return runVerify(argc, argv);
    }

    std::cerr << "Unknown command: " << command << '\n';
    std::cerr << "Run 'startool4 help' for usage information.\n";

    return 1;
}

int StartoolApp::runInspect(int argc, char **argv) const
{
    if (argc != 3) {
        std::cerr << "Usage: startool4 inspect <path>\n";
        return 1;
    }

    const std::filesystem::path inputPath = argv[2];

    std::error_code error;

    if (!std::filesystem::exists(inputPath, error)) {
        if (error) {
            std::cerr
                << "Could not inspect path: "
                << error.message()
                << '\n';
        } else {
            std::cerr
                << "Path does not exist: "
                << inputPath
                << '\n';
        }

        return 1;
    }

    const std::filesystem::path absolutePath = std::filesystem::absolute(inputPath, error);
    if (error) {
        std::cerr
            << "Could not resolve path: "
            << error.message()
            << '\n';

        return 1;
    }

    const std::filesystem::path normalizedPath = absolutePath.lexically_normal();

    SourceDetector detector;
    auto source = detector.detect(normalizedPath);
    if (!source) {
        std::cerr
            << "No supported StarCraft source was detected at:\n"
            << "  "
            << normalizedPath.string()
            << '\n';

        return 1;
    }

    bool verified = false;

    if (source->format == SourceFormat::Mpq) {
        MpqVerifier verifier;
        verified = verifier.verify(*source);
    }

    std::cout
        << "StarCraft source candidate\n"
        << '\n'
        << "Root:    "
        << source->root.string()
        << '\n'
        << "Storage: "
        << source->storage.string()
        << '\n'
        << "Edition: "
        << toString(source->edition)
        << '\n'
        << "Format:  "
        << toString(source->format)
        << '\n'
        << '\n'
        << "Status:  "
        << (verified ? "verified" : "not verified")
        << '\n';

    if (!source->sources.empty()) {
        std::cout
            << '\n'
            << "Logical sources:\n";

        LogicalSourceVerifier logicalVerifier;
        for (const LogicalSource &logicalSource : source->sources) {
            const bool logicalVerified = logicalVerifier.verify(logicalSource);

            std::cout
                << "  "
                << logicalSource.id
                << '\n'
                << "    Format:   "
                << toString(logicalSource.format)
                << '\n'
                << "    Location: "
                << toString(logicalSource.location)
                << '\n'
                << "    Storage:  "
                << logicalSource.storage.string()
                << '\n';

            if (!logicalSource.member.empty()) {
                std::cout
                    << "    Member:   "
                    << logicalSource.member
                    << '\n';
            }

            std::cout
                << "    Status:   "
                << (
                    logicalVerified
                        ? "verified"
                        : "not verified"
                )
                << '\n';
        }
    }

    return 0;
}

int StartoolApp::runExtract(int argc, char **argv) const
{
    if (argc != 6) {
        std::cerr
            << "Usage: startool4 extract "
            << "<installation> <source> <resource> <destination>\n";

        return 1;
    }

    const std::filesystem::path installationPath = argv[2];
    const std::string sourceId = argv[3];
    const std::string resourcePath = argv[4];
    const std::filesystem::path destination = argv[5];

    std::error_code error;

    if (!std::filesystem::exists(installationPath, error)) {
        std::cerr
            << "Installation path does not exist: "
            << installationPath
            << '\n';

        return 1;
    }

    const std::filesystem::path absolutePath = std::filesystem::absolute(installationPath, error);
    if (error) {
        std::cerr
            << "Could not resolve installation path: "
            << error.message()
            << '\n';

        return 1;
    }

    SourceDetector detector;

    auto gameSource = detector.detect(absolutePath.lexically_normal());
    if (!gameSource) {
        std::cerr << "No supported StarCraft source was detected.\n";

        return 1;
    }

    if (gameSource->format != SourceFormat::Mpq) {
        std::cerr << "This source is not an MPQ source.\n";

        return 1;
    }

    MpqVerifier verifier;

    if (!verifier.verify(*gameSource)) {
        std::cerr << "Could not verify the StarCraft MPQ source.\n";

        return 1;
    }

    const auto logicalSource = std::find_if(gameSource->sources.begin(), gameSource->sources.end(), [&sourceId](const LogicalSource &source)
        {
            return source.id == sourceId;
        }
    );

    if (logicalSource == gameSource->sources.end()) {
        std::cerr
            << "Logical source not found: "
            << sourceId
            << '\n';

        return 1;
    }

    MpqSourceReader reader(*logicalSource);
    if (!reader.isOpen()) {
        std::cerr
            << "Could not open logical source: "
            << sourceId
            << '\n';

        return 1;
    }

    if (!reader.contains(resourcePath)) {
        std::cerr
            << "Resource not found: "
            << resourcePath
            << '\n';

        return 1;
    }

    if (std::filesystem::exists(destination, error)) {
        std::cerr
            << "Destination already exists: "
            << destination
            << '\n';

        return 1;
    }

    const std::filesystem::path parentDirectory = destination.parent_path();

    if (!parentDirectory.empty()) {
        std::filesystem::create_directories(
            parentDirectory,
            error
        );

        if (error) {
            std::cerr
                << "Could not create destination directory: "
                << error.message()
                << '\n';

            return 1;
        }
    }

    if (!reader.extract(resourcePath, destination)){
        std::cerr
            << "Could not extract resource: "
            << resourcePath
            << '\n';

        return 1;
    }

    std::cout
        << "Extracted resource\n"
        << '\n'
        << "Source:      "
        << sourceId
        << '\n'
        << "Resource:    "
        << resourcePath
        << '\n'
        << "Destination: "
        << destination.string()
        << '\n';

    return 0;
}

int StartoolApp::runImport(int argc, char **argv) const
{
    if (argc != 5) {
        std::cerr
            << "Usage: startool4 import "
            << "<installation> <manifest> <destination>\n";

        return 1;
    }

    const std::filesystem::path installationPath = argv[2];
    const std::filesystem::path manifestPath = argv[3];
    const std::filesystem::path destinationPath = argv[4];

    ManifestLoader loader;

    std::string manifestError;

    const auto manifest = loader.load(manifestPath, manifestError);
    if (!manifest) {
        std::cerr
            << "Manifest loading failed: "
            << manifestError
            << '\n';

        return 1;
    }

    ImportPlanner planner;
    const std::vector<ImportTask> tasks = planner.plan(*manifest);

    std::error_code error;

    if (!std::filesystem::exists(installationPath, error)) {
        std::cerr
            << "Installation path does not exist: "
            << installationPath
            << '\n';

        return 1;
    }

    const std::filesystem::path absolutePath = std::filesystem::absolute(installationPath, error);

    if (error) {
        std::cerr
            << "Could not resolve installation path: "
            << error.message()
            << '\n';

        return 1;
    }

    SourceDetector detector;
    auto gameSource = detector.detect(absolutePath.lexically_normal());

    if (!gameSource) {
        std::cerr << "No supported StarCraft source was detected\n";
        return 1;
    }

    if (gameSource->format != SourceFormat::Mpq) {
        std::cerr << "Import currently supports only MPQ sources\n";
        return 1;
    }

    MpqVerifier verifier;
    if (!verifier.verify(*gameSource)) {
        std::cerr << "Could not verify the StarCraft MPQ source\n";
        return 1;
    }

    SourceReaderFactory readerFactory;
    SourceReaderRegistry readers;
    ImportExecutor executor;

    std::size_t importedTasks = 0;

    for (const ImportTask &task : tasks) {
        SourceReader *reader = readers.find(task.source);

        if (reader == nullptr) {
            const auto logicalSource =
                std::find_if(
                    gameSource->sources.begin(),
                    gameSource->sources.end(),
                    [&task](const LogicalSource &source)
                    {
                        return source.id == task.source;
                    }
                );

            if (logicalSource == gameSource->sources.end()) {
                std::cerr
                    << "Task '"
                    << task.id
                    << "' references unknown source: "
                    << task.source
                    << '\n';

                return 1;
            }

            auto newReader = readerFactory.create(*logicalSource);
            if (!newReader) {
                std::cerr
                    << "Task '"
                    << task.id
                    << "' references an unsupported source format: "
                    << toString(logicalSource->format)
                    << '\n';

                return 1;
            }

            if (!newReader->isOpen()) {
                std::cerr
                    << "Task '"
                    << task.id
                    << "' could not open source: "
                    << task.source
                    << '\n';

                return 1;
            }

            if (!readers.add(task.source, std::move(newReader))) {
                std::cerr
                    << "Could not register source reader: "
                    << task.source
                    << '\n';

                return 1;
            }

            reader = readers.find(task.source);

            if (reader == nullptr) {
                std::cerr
                    << "Could not retrieve source reader: "
                    << task.source
                    << '\n';

                return 1;
            }
        }

        std::filesystem::path outputPath;
        std::string taskError;

        if (!executor.execute(
                task,
                readers,
                destinationPath,
                outputPath,
                taskError
            ))
        {
            std::cerr
                << "Could not import task '"
                << task.id
                << "': "
                << taskError
                << '\n';

            return 1;
        }

        std::cout
            << "Imported: "
            << task.id
            << '\n'
            << "  Source: "
            << task.input
            << '\n'
            << "  Output: "
            << outputPath.string()
            << '\n';

        ++importedTasks;
    }

    std::cout
        << '\n'
        << "Import completed\n"
        << '\n'
        << "Rules:    "
        << manifest->rules.size()
        << '\n'
        << "Tasks:    "
        << tasks.size()
        << '\n'
        << "Imported: "
        << importedTasks
        << '\n';

    return 0;
}

int StartoolApp::runVerify(int argc, char **argv) const
{
    if (argc != 4) {
        std::cerr << "Usage: startool4 verify <installation> <manifest>\n";
        return 1;
    }

    const std::filesystem::path installationPath = argv[2];
    const std::filesystem::path manifestPath = argv[3];

    ManifestLoader loader;

    std::string manifestError;

    const auto manifest = loader.load(manifestPath, manifestError );
    if (!manifest) {
        std::cerr << "Manifest verification failed: " << manifestError << '\n';
        return 1;
    }

    ImportPlanner planner;
    const std::vector<ImportTask> tasks = planner.plan(*manifest);

    std::error_code error;
    if (!std::filesystem::exists(installationPath, error)) {
        std::cerr << "Installation path does not exist: " << installationPath << '\n';
        return 1;
    }

    const std::filesystem::path absolutePath = std::filesystem::absolute(installationPath, error);
    if (error) {
        std::cerr << "Could not resolve installation path: " << error.message() << '\n';
        return 1;
    }

    SourceDetector detector;

    auto gameSource = detector.detect(absolutePath.lexically_normal());
    if (!gameSource) {
        std::cerr << "No supported StarCraft source was detected\n";
        return 1;
    }

    if (gameSource->format != SourceFormat::Mpq) {
        std::cerr << "Manifest verification currently supports only MPQ sources\n";
        return 1;
    }

    MpqVerifier verifier;
    if (!verifier.verify(*gameSource)) {
        std::cerr << "Could not verify the StarCraft MPQ source\n";
        return 1;
    }

    SourceReaderFactory readerFactory;
    SourceReaderRegistry readers;

    for (const ImportTask &task : tasks) {
        SourceReader *reader = readers.find(task.source);

        if (reader == nullptr) {
            const auto logicalSource =
                std::find_if(
                    gameSource->sources.begin(),
                    gameSource->sources.end(),
                    [&task](const LogicalSource &source)
                    {
                        return source.id == task.source;
                    }
                );

            if (logicalSource == gameSource->sources.end()) {
                std::cerr
                    << "Task '"
                    << task.id
                    << "' references unknown source: "
                    << task.source
                    << '\n';

                return 1;
            }

            auto newReader = readerFactory.create(*logicalSource);
            if (!newReader) {
                std::cerr
                    << "Task '"
                    << task.id
                    << "' references an unsupported source format: "
                    << toString(logicalSource->format)
                    << '\n';

                return 1;
            }

            if (!newReader->isOpen()) {
                std::cerr
                    << "Task '"
                    << task.id
                    << "' could not open source: "
                    << task.source
                    << '\n';

                return 1;
            }

            if (!readers.add(task.source, std::move(newReader))) {
                std::cerr
                    << "Could not register source reader: "
                    << task.source
                    << '\n';

                return 1;
            }

            reader = readers.find(task.source);
            if (reader == nullptr) {
                std::cerr
                    << "Could not retrieve source reader: "
                    << task.source
                    << '\n';

                return 1;
            }
        }

        if (!reader->contains(task.input)) {
            std::cerr
                << "Task '"
                << task.id
                << "' references missing resource: "
                << task.input
                << '\n';

            return 1;
        }
    }

    std::cout
        << "Manifest verified\n"
        << '\n'
        << "Format version: "
        << manifest->formatVersion
        << '\n'
        << "Rules:          "
        << manifest->rules.size()
        << '\n'
        << "Tasks:          "
        << tasks.size()
        << '\n'
        << "Resources:      verified\n";

    return 0;
}

void StartoolApp::printHelp() const
{
    std::cout
        << "Startool - StarCraft data importer for Stargus\n"
        << '\n'
        << "Usage:\n"
        << "  startool4 <command> [options]\n"
        << '\n'
        << "Commands:\n"
        << "  inspect <path>   Inspect a StarCraft installation\n"
        << "  extract          Extract a resource from a StarCraft source\n"
        << "  import           Import StarCraft resources using a manifest\n"
        << "  verify <file>    Verify a manifest against a StarCraft installation\n"
        << "  help             Show this help\n"
        << "  version          Show version information\n"
        << '\n'
        << "Planned commands:\n"
        << "  inspect <path>  Inspect a StarCraft installation\n"
        << "  extract          Extract a resource from a StarCraft source\n"
        << "  help             Show this help\n"
        << "  version          Show version information\n";
}

void StartoolApp::printVersion() const
{
    std::cout << "Startool 4.0.0-dev\n";
}
