// SPDX-License-Identifier: GPL-2.0-only
#include "LauncherImport.h"

#include "GameLocale.h"
#include "MpqVerifier.h"
#include "SourceDetector.h"
#include "LauncherPaths.h"
#ifdef HAVE_CASC
#include "CascVerifier.h"
#endif

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace {
namespace fs = std::filesystem;

std::string environment(const char *name)
{
    const char *value = std::getenv(name);
    return value ? value : "";
}

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

fs::path executableDirectory(const char *argv0)
{
#ifdef _WIN32
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length && length < buffer.size()) return fs::path(std::wstring(buffer.data(), length)).parent_path();
#elif defined(__APPLE__)
    uint32_t length = 0;
    _NSGetExecutablePath(nullptr, &length);
    std::vector<char> buffer(length);
    if (_NSGetExecutablePath(buffer.data(), &length) == 0) return fs::weakly_canonical(buffer.data()).parent_path();
#else
    std::error_code error;
    const auto executable = fs::read_symlink("/proc/self/exe", error);
    if (!error) return executable.parent_path();
#endif
    return fs::absolute(argv0).parent_path();
}

fs::path assetsDirectory(const std::string &explicitPath, const char *argv0)
{
    if (!explicitPath.empty()) return fs::absolute(explicitPath);
    const auto executable = executableDirectory(argv0);
    for (const auto &candidate : {executable, executable / STARTOOL4_RELATIVE_ASSETS,
             fs::path(STARTOOL4_INSTALLED_ASSETS), fs::path(STARTOOL4_SOURCE_ASSETS)}) {
        if (fs::is_directory(candidate / "manifests") && fs::is_directory(candidate / "scripts")) {
            return fs::weakly_canonical(candidate);
        }
    }
    throw std::runtime_error("Could not locate Stargus assets; use --assets or STARTOOL4_ASSETS");
}

bool within(const fs::path &path, const fs::path &root)
{
    auto p = path.begin();
    for (auto r = root.begin(); r != root.end(); ++r, ++p) {
        if (p == path.end() || *p != *r) return false;
    }
    return true;
}

void protectSource(const fs::path &destination, const fs::path &source)
{
    const auto root = fs::weakly_canonical(fs::is_directory(source) ? source : source.parent_path());
    const auto target = fs::weakly_canonical(destination);
    if (within(target, root) || within(root, target)) {
        throw std::runtime_error("The destination must be separate from the installation and asset directories");
    }
}

// A sibling directory is both the import workspace and an exclusive destination
// lock. It is on the destination filesystem, so publish/rollback can use rename.
struct Workspace {
    fs::path root;
    bool retain = false;
    explicit Workspace(const fs::path &destination)
        : root(destination.parent_path() / (destination.filename().string() + ".startool4-lock"))
    {
        fs::create_directories(root.parent_path());
        if (!fs::create_directory(root)) {
            throw std::runtime_error("An import or recovery directory already exists: " + root.string());
        }
    }
    ~Workspace()
    {
        if (!retain) {
            std::error_code ignored;
            fs::remove_all(root, ignored);
        }
    }
};

void copyFile(const fs::path &source, const fs::path &target)
{
    if (!fs::is_regular_file(fs::symlink_status(source))) {
        throw std::runtime_error("Missing or unsupported Stargus asset: " + source.string());
    }
    fs::create_directories(target.parent_path());
    fs::copy_file(source, target, fs::copy_options::overwrite_existing);
}

void copyAssets(const fs::path &assets, const fs::path &stage)
{
    copyFile(assets / "contrib/fog.png", stage / "graphics/tilesets/fog.png");
    copyFile(assets / "contrib/transparent.png", stage / "graphics/ui/transparent.png");
    if (!fs::is_regular_file(assets / "scripts/stratagus.lua")) {
        throw std::runtime_error("Stargus scripts/stratagus.lua is missing");
    }
    for (const auto &entry : fs::recursive_directory_iterator(assets / "scripts")) {
        if (entry.is_directory() && !entry.is_symlink()) continue;
        copyFile(entry.path(), stage / "scripts" / entry.path().lexically_relative(assets / "scripts"));
    }
}

void checkTarget(const fs::path &destination, const fs::path &relative)
{
    fs::path path;
    const auto target = destination / relative;
    for (const auto &component : target) {
        path /= component;
        const auto status = fs::symlink_status(path);
        if (!fs::exists(status)) continue;
        if (fs::is_symlink(status)
            || (path == target ? !fs::is_regular_file(status) : !fs::is_directory(status))) {
            throw std::runtime_error("Unsupported destination entry: " + path.string());
        }
    }
}

void publish(Workspace &workspace, const fs::path &destination)
{
    const auto stage = workspace.root / "data";
    std::vector<fs::path> files;
    for (const auto &entry : fs::recursive_directory_iterator(stage)) {
        if (entry.is_directory() && !entry.is_symlink()) continue;
        if (!fs::is_regular_file(entry.symlink_status())) {
            throw std::runtime_error("Unsupported staged entry: " + entry.path().string());
        }
        const auto relative = entry.path().lexically_relative(stage);
        if (relative != "extracted") files.push_back(relative);
    }
    std::sort(files.begin(), files.end());
    files.emplace_back("extracted"); // version marker is committed last
    for (const auto &relative : files) checkTarget(destination, relative);

    struct Change { fs::path relative; bool backedUp = false; bool installed = false; };
    std::vector<Change> changes;
    changes.reserve(files.size());
    try {
        for (const auto &relative : files) {
            changes.push_back({relative, false, false});
            auto &change = changes.back();
            const auto target = destination / relative;
            const auto backup = workspace.root / "backup" / relative;
            fs::create_directories(target.parent_path());
            if (fs::exists(target)) {
                fs::create_directories(backup.parent_path());
                fs::rename(target, backup);
                change.backedUp = true;
            }
            fs::rename(stage / relative, target);
            change.installed = true;
        }
    } catch (...) {
        bool recovered = true;
        for (auto it = changes.rbegin(); it != changes.rend(); ++it) {
            std::error_code error;
            const auto target = destination / it->relative;
            if (it->installed) {
                fs::remove(target, error);
                if (error) recovered = false;
            }
            if (it->backedUp) {
                error.clear();
                fs::rename(workspace.root / "backup" / it->relative, target, error);
                if (error) recovered = false;
            }
        }
        if (!recovered) {
            workspace.retain = true;
            std::cerr << "Recovery files retained at " << workspace.root.string() << '\n';
        }
        throw;
    }
}
} // namespace

int runLauncherImport(int argc, char **argv,
    const std::function<int(std::vector<std::string>)> &importer)
{
    if (argc < 4 || (argc - 4) % 2) {
        std::cerr << "Usage: startool4 launcher <installation> <destination> "
                     "[--base <classic>] [--locale <locale>] [--edition classic|broodwar] [--assets <directory>]\n";
        return 1;
    }
    try {
        std::map<std::string, std::string> options;
        for (int i = 4; i < argc; i += 2) {
            const std::string key = argv[i];
            if (key != "--base" && key != "--locale" && key != "--edition" && key != "--assets") {
                throw std::runtime_error("Unknown launcher option: " + key);
            }
            if (!options.emplace(key, argv[i + 1]).second || options.at(key).empty()) {
                throw std::runtime_error("Empty or repeated launcher option: " + key);
            }
        }
        const auto option = [&](const char *key, const char *variable) {
            const auto it = options.find(key);
            return it == options.end() ? environment(variable) : it->second;
        };
        const auto requestedEdition = option("--edition", "STARTOOL4_EDITION");
        if (!requestedEdition.empty() && requestedEdition != "classic" && requestedEdition != "broodwar") {
            throw std::runtime_error("Edition must be classic or broodwar");
        }
        auto locale = option("--locale", "STARTOOL4_LOCALE");
        if (!locale.empty() && !parseGameLocale(locale)) throw std::runtime_error("Unsupported locale: " + locale);
        fs::path source = fs::absolute(argv[2]).lexically_normal();
        if (lower(source.filename().string()) == ".build.info") source = source.parent_path();
        auto destination = fs::absolute(argv[3]).lexically_normal();
        if (destination.filename().empty()) destination = destination.parent_path();
        if (destination == destination.root_path()) throw std::runtime_error("The destination cannot be a filesystem root");
        // Resolve parent aliases (e.g. macOS /var) but retain the final entry so
        // publish still rejects a symlink used as the destination itself.
        destination = fs::weakly_canonical(destination.parent_path()) / destination.filename();
        if (!fs::exists(source)) throw std::runtime_error("Installation path does not exist: " + source.string());
        SourceDetector detector;
        auto detected = detector.detect(source);
        if (!detected) throw std::runtime_error("No supported StarCraft installation was detected");
        bool verified = false;
        if (detected->format == SourceFormat::Mpq) {
            MpqVerifier verifier;
            verified = verifier.verify(*detected);
        } else if (detected->format == SourceFormat::Casc) {
#ifdef HAVE_CASC
            CascVerifier verifier;
            verified = verifier.verify(*detected);
#else
            throw std::runtime_error("This Startool4 build has no CASC support");
#endif
        }
        if (!verified) throw std::runtime_error("Could not verify the StarCraft installation");
        std::string edition = detected->edition == GameEdition::BroodWar ? "broodwar" : "classic";
        if (detected->format == SourceFormat::Casc) {
            if (!requestedEdition.empty()) edition = requestedEdition;
            if (locale.empty()) locale = "enUS";
        } else {
            if (!locale.empty()) throw std::runtime_error("--locale / STARTOOL4_LOCALE applies only to CASC");
            if (!requestedEdition.empty() && requestedEdition != edition) {
                throw std::runtime_error("Requested edition does not match the MPQ installation");
            }
        }
        auto base = option("--base", "STARTOOL4_BASE");
        if (detected->format == SourceFormat::Mpq && edition == "broodwar" && base.empty()) {
            for (const auto &entry : fs::directory_iterator(detected->root)) {
                if (entry.is_regular_file() && lower(entry.path().filename().string()) == "stardat.mpq") {
                    base = entry.path().string();
                    break;
                }
            }
            if (base.empty()) throw std::runtime_error("Brood War needs --base or STARTOOL4_BASE pointing to Classic");
        } else if (!base.empty() && (detected->format != SourceFormat::Mpq || edition != "broodwar")) {
            throw std::runtime_error("--base / STARTOOL4_BASE applies only to Brood War MPQ");
        }
        const auto assets = assetsDirectory(option("--assets", "STARTOOL4_ASSETS"), argv[0]);
        const auto manifest = assets / "manifests" /
            ((detected->format == SourceFormat::Casc ? "casc-" : "") + edition + ".json");
        if (!fs::is_regular_file(manifest)) throw std::runtime_error("Manifest missing: " + manifest.string());
        protectSource(destination, detected->root);
        // Portable packages keep their scripts/manifests in the data directory.
        // All asset reads finish in staging before any such files are replaced.
        if (fs::weakly_canonical(destination) != fs::weakly_canonical(assets)) {
            protectSource(destination, assets);
        }
        if (!base.empty()) protectSource(destination, fs::absolute(base));
        Workspace workspace(destination);
        const auto stage = workspace.root / "data";
        std::vector<std::string> arguments = {argv[0], "import", source.string(), manifest.string(), stage.string()};
        if (!base.empty()) {
            arguments.emplace_back("--base");
            arguments.push_back(base);
        }
        if (!locale.empty()) {
            arguments.emplace_back("--locale");
            arguments.push_back(locale);
        }
        std::cout << "Preparing Stargus " << edition << " data using " << manifest.string() << '\n';
        const int result = importer(std::move(arguments));
        if (result != 0) return result;
        copyAssets(assets, stage);
        std::ofstream marker(stage / "extracted", std::ios::binary);
        marker << StartoolDataVersion << '\n';
        marker.close();
        if (!marker) throw std::runtime_error("Could not write the extraction version marker");
        publish(workspace, destination);
        std::cout << "Stargus data prepared at " << destination.string() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Stargus import failed: " << error.what() << '\n';
        return 1;
    }
}
