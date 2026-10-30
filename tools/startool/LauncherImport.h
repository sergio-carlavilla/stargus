// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include <functional>
#include <string>
#include <vector>

inline constexpr char StartoolDataVersion[] = "4.0.0-dev.1";

// Adapt the two paths supplied by stratagus-game-launcher.h to Startool 4.
// The callback runs the normal importer, without a shell or a second binary.
int runLauncherImport(int argc, char **argv,
    const std::function<int(std::vector<std::string>)> &importer);
