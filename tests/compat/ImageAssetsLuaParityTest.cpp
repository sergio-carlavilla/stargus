#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
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

    if (!stream.good() && !stream.eof()) {
        throw std::runtime_error("Could not read file: " + path.string());
    }

    return buffer.str();
}

std::vector<std::string> splitLines(const std::string& text)
{
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }

    return lines;
}

void printFirstDifference(
    const std::string& reference,
    const std::string& candidate)
{
    const auto referenceLines = splitLines(reference);
    const auto candidateLines = splitLines(candidate);
    const std::size_t commonLines =
        std::min(referenceLines.size(), candidateLines.size());

    for (std::size_t i = 0; i < commonLines; ++i) {
        if (referenceLines[i] != candidateLines[i]) {
            std::cerr
                << "  First differing line: " << (i + 1) << '\n'
                << "  StarTool3: " << referenceLines[i] << '\n'
                << "  StarTool4: " << candidateLines[i] << '\n';
            return;
        }
    }

    if (referenceLines.size() != candidateLines.size()) {
        std::cerr
            << "  Line count differs\n"
            << "  StarTool3: " << referenceLines.size() << " lines\n"
            << "  StarTool4: " << candidateLines.size() << " lines\n";
        return;
    }

    const std::size_t commonBytes = std::min(reference.size(), candidate.size());

    for (std::size_t i = 0; i < commonBytes; ++i) {
        if (reference[i] != candidate[i]) {
            std::cerr
                << "  First differing byte: " << i << '\n'
                << "  StarTool3 byte: "
                << static_cast<unsigned int>(
                       static_cast<unsigned char>(reference[i]))
                << '\n'
                << "  StarTool4 byte: "
                << static_cast<unsigned int>(
                       static_cast<unsigned char>(candidate[i]))
                << '\n';
            return;
        }
    }

    std::cerr
        << "  File size differs\n"
        << "  StarTool3: " << reference.size() << " bytes\n"
        << "  StarTool4: " << candidate.size() << " bytes\n";
}

bool compareLua(
    const fs::path& referencePath,
    const fs::path& candidatePath)
{
    if (!fs::exists(referencePath)) {
        std::cerr
            << "FAIL: StarTool3 reference does not exist:\n"
            << "  " << referencePath << '\n';
        return false;
    }

    const std::string reference = readFile(referencePath);
    const std::string candidate = readFile(candidatePath);

    if (reference == candidate) {
        return true;
    }

    std::cerr
        << "FAIL: Lua output differs\n"
        << "  StarTool3: " << referencePath << '\n'
        << "  StarTool4: " << candidatePath << '\n';

    printFirstDifference(reference, candidate);
    return false;
}

std::vector<fs::path> collectGeneratedImageLuaFiles(const fs::path& outputDir)
{
    const fs::path root = outputDir / "luagen/images";

    if (!fs::exists(root)) {
        throw std::runtime_error(
            "StarTool4 did not generate luagen/images: " + root.string());
    }

    std::vector<fs::path> files;

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (entry.path().extension() != ".lua") {
            continue;
        }

        files.push_back(fs::relative(entry.path(), outputDir));
    }

    std::sort(files.begin(), files.end());
    return files;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::cerr
            << "Usage:\n"
            << "  Startool4ImageAssetsLuaParityTest "
            << "<startool4> "
            << "<starcraft-dir> "
            << "<manifest> "
            << "<startool3-reference-dir>\n";
        return EXIT_FAILURE;
    }

    const fs::path startool4 = argv[1];
    const fs::path starcraftDir = argv[2];
    const fs::path manifest = argv[3];
    const fs::path referenceDir = argv[4];

    const fs::path outputDir =
        fs::temp_directory_path() /
        "startool4-image-assets-lua-parity";

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

    const int result = std::system(command.c_str());

    if (result != 0) {
        std::cerr
            << "FAIL: StarTool4 import returned "
            << result << '\n';
        return EXIT_FAILURE;
    }

    std::size_t checked = 0;
    std::size_t failed = 0;

    try {
        const auto assets = collectGeneratedImageLuaFiles(outputDir);

        if (assets.empty()) {
            std::cerr << "FAIL: no image Lua files were generated\n";
            return EXIT_FAILURE;
        }

        for (const fs::path& relativePath : assets) {
            std::cout << "Checking: " << relativePath << '\n';

            try {
                if (!compareLua(
                        referenceDir / relativePath,
                        outputDir / relativePath)) {
                    ++failed;
                } else {
                    std::cout << "  IDENTICAL\n";
                }
            } catch (const std::exception& exception) {
                std::cerr << "FAIL: " << exception.what() << '\n';
                ++failed;
            }

            ++checked;
        }
    } catch (const std::exception& exception) {
        std::cerr << "FAIL: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout
        << "\nChecked: " << checked
        << "\nFailed:  " << failed
        << '\n';

    if (failed != 0) {
        std::cerr << "IMAGE ASSETS LUA PARITY FAILED\n";
        return EXIT_FAILURE;
    }

    std::cout
        << "ALL GENERATED IMAGE ASSET LUA FILES IDENTICAL TO STARTOOL3\n";

    fs::remove_all(outputDir);
    return EXIT_SUCCESS;
}
