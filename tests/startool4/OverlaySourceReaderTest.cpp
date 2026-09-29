// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "OverlaySourceReader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    class FakeSourceReader : public SourceReader
    {
        public:
            explicit FakeSourceReader(std::map<std::string, std::string> resources)
                : resources_(std::move(resources))
            {
            }

            bool isOpen() const override
            {
                return true;
            }

            bool contains(std::string_view resource) const override
            {
                return resources_.find(std::string(resource)) != resources_.end();
            }

            bool extract(
                std::string_view resource,
                const std::filesystem::path &destination
            ) const override
            {
                const auto iterator = resources_.find(std::string(resource));

                if (iterator == resources_.end()) {
                    return false;
                }

                std::ofstream output(destination, std::ios::binary);
                if (!output) {
                    return false;
                }

                output << iterator->second;
                return static_cast<bool>(output);
            }

        private:
            std::map<std::string, std::string> resources_;
    };

    std::string readFile(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        return std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );
    }

    bool expectExtracted(
        const OverlaySourceReader &reader,
        std::string_view resource,
        std::string_view expected
    )
    {
        std::error_code error;
        const std::filesystem::path output =
            std::filesystem::temp_directory_path(error) /
            "startool4-overlay-source-reader-test.bin";

        if (error) {
            return false;
        }

        std::filesystem::remove(output, error);
        error.clear();

        if (!reader.extract(resource, output)) {
            return false;
        }

        const std::string content = readFile(output);
        std::filesystem::remove(output, error);

        return content == expected;
    }
}

int main()
{
    auto primary = std::make_unique<FakeSourceReader>(
        std::map<std::string, std::string>{
            {"broodwar-only", "broodwar"},
            {"shared", "broodwar-override"}
        }
    );

    auto fallback = std::make_unique<FakeSourceReader>(
        std::map<std::string, std::string>{
            {"classic-only", "classic"},
            {"shared", "classic-base"}
        }
    );

    OverlaySourceReader overlay(
        std::move(primary),
        std::move(fallback)
    );

    if (!overlay.isOpen()) {
        std::cerr << "Overlay reader is not open\n";
        return 1;
    }

    if (!overlay.contains("classic-only")) {
        std::cerr << "Classic fallback resource was not found\n";
        return 1;
    }

    if (!overlay.contains("broodwar-only")) {
        std::cerr << "Brood War resource was not found\n";
        return 1;
    }

    if (!overlay.contains("shared")) {
        std::cerr << "Shared resource was not found\n";
        return 1;
    }

    if (overlay.contains("missing")) {
        std::cerr << "Missing resource was reported as present\n";
        return 1;
    }

    if (!expectExtracted(overlay, "classic-only", "classic")) {
        std::cerr << "Classic fallback extraction failed\n";
        return 1;
    }

    if (!expectExtracted(overlay, "broodwar-only", "broodwar")) {
        std::cerr << "Brood War extraction failed\n";
        return 1;
    }

    if (!expectExtracted(overlay, "shared", "broodwar-override")) {
        std::cerr << "Brood War did not override Classic for shared resource\n";
        return 1;
    }

    return 0;
}
