#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string shellQuote(const std::string& value)
{
    std::string result = "'";

    for (const char c : value) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }

    result += '\'';
    return result;
}

std::string readFile(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);

    if (!stream) {
        throw std::runtime_error("Could not open file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::set<fs::path> collectRelativeFiles(const fs::path& root)
{
    std::set<fs::path> result;

    if (!fs::exists(root)) {
        return result;
    }

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        result.insert(fs::relative(entry.path(), root));
    }

    return result;
}

std::vector<int> extractImageIds(const std::set<fs::path>& files)
{
    static const std::regex pattern(R"(^image_([0-9]+)_.*\.lua$)");

    std::set<int> ids;

    for (const auto& file : files) {
        std::smatch match;
        const std::string filename = file.filename().string();

        if (std::regex_match(filename, match, pattern)) {
            ids.insert(std::stoi(match[1].str()));
        }
    }

    return {ids.begin(), ids.end()};
}

void printPaths(const std::string& title, const std::set<fs::path>& paths)
{
    std::cout << '\n' << title << " (" << paths.size() << ")\n";

    if (paths.empty()) {
        std::cout << "  (none)\n";
        return;
    }

    for (const auto& path : paths) {
        std::cout << "  " << path.generic_string() << '\n';
    }
}

void printIds(const std::string& title, const std::vector<int>& ids)
{
    std::cout << '\n' << title << " (" << ids.size() << ")\n";

    if (ids.empty()) {
        std::cout << "  (none)\n";
        return;
    }

    constexpr std::size_t perLine = 16;

    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i % perLine == 0) {
            std::cout << "  ";
        }

        std::cout << ids[i];

        if (i + 1 != ids.size()) {
            std::cout << ", ";
        }

        if ((i + 1) % perLine == 0 || i + 1 == ids.size()) {
            std::cout << '\n';
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::cerr
            << "Usage:\n"
            << "  Startool4ImageAssetsCoverageReport "
            << "<startool4> <starcraft-dir> <manifest> <startool3-reference-dir>\n";
        return EXIT_FAILURE;
    }

    const fs::path startool4 = argv[1];
    const fs::path starcraftDir = argv[2];
    const fs::path manifest = argv[3];
    const fs::path referenceDir = argv[4];

    const fs::path referenceImagesDir = referenceDir / "luagen/images";

    if (!fs::is_directory(referenceImagesDir)) {
        std::cerr
            << "FAIL: StarTool3 image Lua directory does not exist:\n"
            << "  " << referenceImagesDir << '\n';
        return EXIT_FAILURE;
    }

    const fs::path outputDir =
        fs::temp_directory_path() / "startool4-image-assets-coverage";

    fs::remove_all(outputDir);
    fs::create_directories(outputDir);

    const std::string command =
        shellQuote(startool4.string()) +
        " import " +
        shellQuote(starcraftDir.string()) +
        " " +
        shellQuote(manifest.string()) +
        " " +
        shellQuote(outputDir.string());

    std::cout << "Running StarTool4 import...\n";

    const int importResult = std::system(command.c_str());

    if (importResult != 0) {
        std::cerr
            << "FAIL: StarTool4 import returned "
            << importResult << '\n';
        return EXIT_FAILURE;
    }

    const fs::path candidateImagesDir = outputDir / "luagen/images";

    const std::set<fs::path> referenceFiles =
        collectRelativeFiles(referenceImagesDir);
    const std::set<fs::path> candidateFiles =
        collectRelativeFiles(candidateImagesDir);

    std::set<fs::path> missing;
    std::set<fs::path> extra;
    std::set<fs::path> common;
    std::set<fs::path> different;

    std::set_difference(
        referenceFiles.begin(), referenceFiles.end(),
        candidateFiles.begin(), candidateFiles.end(),
        std::inserter(missing, missing.end()));

    std::set_difference(
        candidateFiles.begin(), candidateFiles.end(),
        referenceFiles.begin(), referenceFiles.end(),
        std::inserter(extra, extra.end()));

    std::set_intersection(
        referenceFiles.begin(), referenceFiles.end(),
        candidateFiles.begin(), candidateFiles.end(),
        std::inserter(common, common.end()));

    for (const auto& relativePath : common) {
        if (readFile(referenceImagesDir / relativePath) !=
            readFile(candidateImagesDir / relativePath)) {
            different.insert(relativePath);
        }
    }

    const std::vector<int> referenceIds = extractImageIds(referenceFiles);
    const std::vector<int> candidateIds = extractImageIds(candidateFiles);
    const std::vector<int> missingIds = extractImageIds(missing);
    const std::vector<int> extraIds = extractImageIds(extra);

    std::cout
        << "\n=== Images.dat Lua coverage ===\n"
        << "StarTool3 files: " << referenceFiles.size() << '\n'
        << "StarTool4 files: " << candidateFiles.size() << '\n'
        << "Common files:    " << common.size() << '\n'
        << "Missing files:   " << missing.size() << '\n'
        << "Extra files:     " << extra.size() << '\n'
        << "Different files: " << different.size() << '\n'
        << "StarTool3 IDs:   " << referenceIds.size() << '\n'
        << "StarTool4 IDs:   " << candidateIds.size() << '\n';

    printIds("Missing Images.dat IDs", missingIds);
    printIds("Extra Images.dat IDs", extraIds);
    printPaths("Missing Lua files", missing);
    printPaths("Extra Lua files", extra);
    printPaths("Different common Lua files", different);

    if (missing.empty() && extra.empty() && different.empty()) {
        std::cout << "\nFULL IMAGE LUA COVERAGE MATCHES STARTOOL3\n";
    } else {
        std::cout
            << "\nCoverage report complete. Differences are expected until "
            << "StarTool4 reaches full StarTool3 Images.dat parity.\n";
    }

    fs::remove_all(outputDir);
    return EXIT_SUCCESS;
}
