# SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 4.3)

project(
    stargus
    VERSION 2.4.1
    DESCRIPTION "StarCraft game module for Stratagus"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------

option(WITH_CASCLIB "Compile startool with CascLib support" ON)

if(WIN32)
    option(ENABLE_NSIS "Create Stargus Windows NSIS installer" OFF)
endif()

# ---------------------------------------------------------------------------
# Windows setup
# ---------------------------------------------------------------------------

if(WIN32 AND MSVC)
    if(WIN32_CMAKE_PREFIX_PATH AND EXISTS "${WIN32_CMAKE_PREFIX_PATH}")
        list(APPEND CMAKE_PREFIX_PATH "${WIN32_CMAKE_PREFIX_PATH}")
        message(STATUS "Using prefix path: ${CMAKE_PREFIX_PATH}")
    elseif(NOT CMAKE_PREFIX_PATH)
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies")
    endif()
endif()

if(WIN32)
    enable_language(RC)
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

find_package(Stratagus REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(StormLib REQUIRED)

if(WITH_CASCLIB)
    find_package(CascLib REQUIRED)
endif()

if(WIN32 AND ENABLE_NSIS)
    find_program(
        MAKENSIS_EXECUTABLE
        NAMES makensis
        HINTS
            "C:/Program Files/NSIS"
            "C:/Program Files (x86)/NSIS"
        REQUIRED
    )

    set(
        MAKENSIS_FLAGS
        ""
        CACHE STRING
        "Additional command-line flags passed to makensis"
    )
endif()

find_package(nlohmann_json 3.11.2 QUIET)

if(NOT nlohmann_json_FOUND)
    add_subdirectory(subprojects/nlohmann_json EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
    message(FATAL_ERROR "nlohmann_json target was not created")
endif()

if(NOT WIN32)
    find_program(
        IMAGEMAGICK_CONVERT
        NAMES magick convert
        REQUIRED
    )
endif()

# ---------------------------------------------------------------------------
# starformat
# ---------------------------------------------------------------------------
set(STARFORMAT_SOURCES
    src/AbstractPalette.cpp
    src/Breeze.cpp
    src/Casc.cpp
    src/Chk.cpp
    src/Color.cpp
    src/Converter.cpp
    src/DataChunk.cpp
    src/Dds.cpp
    src/FileNotFoundException.cpp
    src/FileUtil.cpp
    src/Font.cpp
    src/Grp.cpp
    src/Hurricane.cpp
    src/ImagesConverter.cpp
    src/KaitaiConverter.cpp
    src/NoValidPaletteException.cpp
    src/Palette.cpp
    src/Palette2D.cpp
    src/PaletteImage.cpp
    src/Panel.cpp
    src/Pcx.cpp
    src/PngExporter.cpp
    src/PortraitsConverter.cpp
    src/Preferences.cpp
    src/Scm.cpp
    src/SfxConverter.cpp
    src/Smacker.cpp
    src/Storage.cpp
    src/Storm.cpp
    src/StringUtil.cpp
    src/UIConsole.cpp
    src/UnitsConverter.cpp
    src/Wav.cpp
    src/Widgets.cpp
    src/WorldMap.cpp
    src/luagen.cpp
    src/pacman.cpp
    src/platform.cpp

    src/dat/DataHub.cpp
    src/dat/Flingy.cpp
    src/dat/IScript.cpp
    src/dat/Image.cpp
    src/dat/Order.cpp
    src/dat/Portrait.cpp
    src/dat/PropertyNotAvailableException.cpp
    src/dat/Sfx.cpp
    src/dat/Sprite.cpp
    src/dat/Tbl.cpp
    src/dat/TblEntry.cpp
    src/dat/Techdata.cpp
    src/dat/Unit.cpp
    src/dat/Upgrade.cpp
    src/dat/Weapon.cpp

    src/kaitai/file_tbl.cpp
    src/kaitai/flingy_dat.cpp
    src/kaitai/images_dat.cpp
    src/kaitai/iscript_bin.cpp
    src/kaitai/kaitaistream.cpp
    src/kaitai/mapdata_dat.cpp
    src/kaitai/opcode_list_type.cpp
    src/kaitai/orders_dat.cpp
    src/kaitai/portdata_dat.cpp
    src/kaitai/sfxdata_dat.cpp
    src/kaitai/sprites_dat.cpp
    src/kaitai/techdata_dat.cpp
    src/kaitai/tileset_cv5.cpp
    src/kaitai/tileset_dddata_bin.cpp
    src/kaitai/tileset_vf4.cpp
    src/kaitai/tileset_vr4.cpp
    src/kaitai/tileset_vx4.cpp
    src/kaitai/units_dat.cpp
    src/kaitai/upgrades_dat.cpp
    src/kaitai/weapons_dat.cpp

    src/libgrp/Exceptions/GRPException.cpp
    src/libgrp/Exceptions/GRPFrame/GRPFrameException.cpp
    src/libgrp/Exceptions/GRPImage/GRPImageException.cpp
    src/libgrp/GRPFrame/GRPFrame.cpp
    src/libgrp/GRPImage/GRPImage.cpp
    src/libgrp/libgrp.cpp

    src/tileset/MegaTile.cpp
    src/tileset/TiledPaletteImage.cpp
    src/tileset/TilesetHub.cpp
)

add_library(
    starformat
    STATIC
    ${STARFORMAT_SOURCES}
)

target_include_directories(
    starformat
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(
    starformat
    PRIVATE
        KS_STR_ENCODING_NONE
)

target_link_libraries(
    starformat
    PUBLIC
        PNG::PNG
        nlohmann_json::nlohmann_json

    PRIVATE
        ZLIB::ZLIB
        BZip2::BZip2
        StormLib::StormLib
)

if(WITH_CASCLIB)
    target_compile_definitions(
        starformat
        PUBLIC
            HAVE_CASC
    )

    target_link_libraries(
        starformat
        PRIVATE
            CascLib::CascLib
    )
endif()

if(WIN32 AND MSVC)
    target_compile_definitions(
        starformat
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
            _CRT_SECURE_NO_DEPRECATE=1
            _HAS_STD_BYTE=0
    )

    add_subdirectory(subprojects/win-iconv EXCLUDE_FROM_ALL)

    target_link_libraries(
        starformat
        PRIVATE
            iconv-static
    )
endif()

# ---------------------------------------------------------------------------
# startool
# ---------------------------------------------------------------------------

add_executable(
    startool
    src/startool.cpp
)

target_link_libraries(
    startool
    PRIVATE
        starformat
)

install(
    TARGETS startool
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# ---------------------------------------------------------------------------
# Stargus launcher
# ---------------------------------------------------------------------------

set(STARGUS_SOURCES
    src/stargus.cpp
)

if(WIN32)
    list(APPEND STARGUS_SOURCES stargus.rc)
endif()

add_executable(
    stargus
    WIN32
    ${STARGUS_SOURCES}
)

target_link_libraries(
    stargus
    PRIVATE
        Stratagus::GameHeaders
)

set(
    STARGUS_GAME_BINDIR
    "games"
    CACHE PATH
    "Where to install the Stargus launcher"
)

set(
    STARGUS_DATA_DIR
    "${CMAKE_INSTALL_DATADIR}/games/stratagus/stargus"
    CACHE PATH
    "Where to install Stargus game data"
)

set(
    STARGUS_ICON_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/pixmaps"
    CACHE PATH
    "Where to install Stargus icons"
)

set(
    STARGUS_DESKTOP_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    CACHE PATH
    "Where to install the Stargus desktop file"
)

if(IS_ABSOLUTE "${STARGUS_DATA_DIR}")
    set(STARGUS_DATA_PATH "${STARGUS_DATA_DIR}")
else()
    set(STARGUS_DATA_PATH "${CMAKE_INSTALL_PREFIX}/${STARGUS_DATA_DIR}")
endif()

target_compile_definitions(
    stargus
    PRIVATE
        SOURCE_DIR="${PROJECT_SOURCE_DIR}/"
        DATA_PATH="${STARGUS_DATA_PATH}"
        SCRIPTS_PATH="${STARGUS_DATA_PATH}"
        STRATAGUS_BIN="${Stratagus_EXECUTABLE}"
)

install(
    TARGETS stargus
    RUNTIME DESTINATION "${STARGUS_GAME_BINDIR}"
)

# ---------------------------------------------------------------------------
# UNIX desktop integration
# ---------------------------------------------------------------------------

if(NOT WIN32)
    configure_file(
        "${PROJECT_SOURCE_DIR}/stargus.desktop.in"
        "${PROJECT_BINARY_DIR}/stargus.desktop"
        @ONLY
    )

    add_custom_command(
        OUTPUT "${PROJECT_BINARY_DIR}/stargus.png"
        COMMAND
            "${IMAGEMAGICK_CONVERT}"
            "${PROJECT_SOURCE_DIR}/stargus.ico"
            "${PROJECT_BINARY_DIR}/stargus.png"
        DEPENDS "${PROJECT_SOURCE_DIR}/stargus.ico"
        COMMENT "Generating stargus.png"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${PROJECT_BINARY_DIR}/stargus.xpm"
        COMMAND
            "${IMAGEMAGICK_CONVERT}"
            "${PROJECT_SOURCE_DIR}/stargus.ico"
            -resize 32x32
            "${PROJECT_BINARY_DIR}/stargus.xpm"
        DEPENDS "${PROJECT_SOURCE_DIR}/stargus.ico"
        COMMENT "Generating stargus.xpm"
        VERBATIM
    )

    add_custom_target(
        stargus-icons
        ALL
        DEPENDS
            "${PROJECT_BINARY_DIR}/stargus.png"
            "${PROJECT_BINARY_DIR}/stargus.xpm"
    )

    install(
        FILES
            "${PROJECT_BINARY_DIR}/stargus.png"
            "${PROJECT_BINARY_DIR}/stargus.xpm"
        DESTINATION "${STARGUS_ICON_DIR}"
    )

    install(
        FILES
            "${PROJECT_BINARY_DIR}/stargus.desktop"
        DESTINATION "${STARGUS_DESKTOP_DIR}"
    )

    install(
        DIRECTORY
            campaigns
            contrib
            maps
            scripts
            shaders
        DESTINATION "${STARGUS_DATA_DIR}"
    )

    install(
        FILES
            mpqlist.txt
        DESTINATION "${STARGUS_DATA_DIR}"
    )

    install(
        FILES
            dataset/palettes.json
            dataset/units.json
        DESTINATION "${STARGUS_DATA_DIR}/dataset"
    )
endif()

# ---------------------------------------------------------------------------
# Windows NSIS installer
# ---------------------------------------------------------------------------

if(WIN32 AND ENABLE_NSIS)
    separate_arguments(
        MAKENSIS_FLAGS_LIST
        NATIVE_COMMAND
        "${MAKENSIS_FLAGS}"
    )

    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/COPYING"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
    )
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/stargus.ico"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
    )
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mpqlist.txt"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
    )

    add_custom_target(
        nsis
        ALL
        COMMAND
            "${MAKENSIS_EXECUTABLE}"
            ${MAKENSIS_FLAGS_LIST}
            -DPORTABLE=1
            -DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/stargus.nsi"
        COMMAND
            "${MAKENSIS_EXECUTABLE}"
            ${MAKENSIS_FLAGS_LIST}
            -DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/stargus.nsi"
        DEPENDS
            stargus
            startool
            "${CMAKE_CURRENT_SOURCE_DIR}/stargus.nsi"
            "${CMAKE_CURRENT_SOURCE_DIR}/COPYING"
            "${CMAKE_CURRENT_SOURCE_DIR}/mpqlist.txt"
            "${CMAKE_CURRENT_SOURCE_DIR}/stargus.ico"
        COMMENT "Generating Stargus Windows NSIS installers"
        VERBATIM
    )
endif()

# ---------------------------------------------------------------------------
# Uninstall target
# ---------------------------------------------------------------------------

if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE
        @ONLY
    )

    add_custom_target(
        uninstall
        COMMAND
            "${CMAKE_COMMAND}"
            -P
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    )
endif()
