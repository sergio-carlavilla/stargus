// SPDX-FileCopyrightText: 2010-2012 Pali Rohár <pali.rohar@gmail.com>
// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include <iostream>

#define GAME_NAME "Stargus"
#define GAME_CD "StarCraft CD or Battle.net installation"
#define GAME_CD_FILE_PATTERNS "install.exe", "Install.exe", "INSTALL.EXE", "stardat.mpq", "StarDat.mpq", "broodat.mpq", "BrooDat.mpq", "starcraft.mpq", "StarCraft.mpq", ".build.info"
#define GAME "stargus"
#define EXTRACTOR_TOOL "startool4"
#define EXTRACTOR_ARGS { "launcher", NULL }
#define CHECK_EXTRACTED_VERSION 1

// Startool4 stages these assets together with the import and publishes them
// only after success, including the explicit-file extraction mode.
#define CONTRIB_DIRECTORIES { NULL }

const char *SRC_PATH()
{
  std::cout << "sourcedir:" << SOURCE_DIR << std::endl;
  return SOURCE_DIR;
}
