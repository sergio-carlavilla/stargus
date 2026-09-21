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
include(CTest)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------

option(WITH_CASCLIB "Compile startool with CascLib support" ON)

if(WIN32)
    option(ENABLE_NSIS "Create Stargus Windows NSIS installer" OFF)
endif()

# ---------------------------------------------------------------------------
# Installation layout
# ---------------------------------------------------------------------------

set(
    STARGUS_GAME_BINDIR
    "games"
    CACHE PATH
    "Stargus launcher path"
)

set(
    STARGUS_DATA_DIR
    "${CMAKE_INSTALL_DATADIR}/games/stratagus/stargus"
    CACHE PATH
    "Stargus game data"
)

set(
    STARGUS_ICON_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/pixmaps"
    CACHE PATH
    "Stargus icons"
)

set(
    STARGUS_DESKTOP_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    CACHE PATH
    "Stargus desktop file"
)

if(IS_ABSOLUTE "${STARGUS_GAME_BINDIR}")
    set(GAMEDIRABS "${STARGUS_GAME_BINDIR}")
else()
    set(GAMEDIRABS "${CMAKE_INSTALL_PREFIX}/${STARGUS_GAME_BINDIR}")
endif()

if(IS_ABSOLUTE "${STARGUS_DATA_DIR}")
    set(STARGUS_DATA_PATH "${STARGUS_DATA_DIR}")
else()
    set(STARGUS_DATA_PATH "${CMAKE_INSTALL_PREFIX}/${STARGUS_DATA_DIR}")
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

find_package(Stratagus QUIET)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(StormLib REQUIRED)
find_package(nlohmann_json 3.11.2 REQUIRED)
find_package(Vorbis REQUIRED)

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

find_package(ImageMagick COMPONENTS Magick++ QUIET)

if(NOT WIN32)
    find_program(
        IMAGEMAGICK_CONVERT
        NAMES magick convert
        REQUIRED
    )
endif()

find_package(Iconv REQUIRED)
set(STARGUS_ICONV_TARGET Iconv::Iconv)

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_check_modules(LOG4CXX QUIET IMPORTED_TARGET liblog4cxx)

    if(BUILD_TESTING)
        pkg_check_modules(CPPUNIT QUIET IMPORTED_TARGET cppunit)
    endif()
endif()

set(STARGUS_CPPUNIT_TARGET "")

if(BUILD_TESTING)
    if(TARGET PkgConfig::CPPUNIT)
        set(STARGUS_CPPUNIT_TARGET PkgConfig::CPPUNIT)
    else()
        find_path(
            CPPUNIT_INCLUDE_DIR
            NAMES cppunit/TestFixture.h
        )

        find_library(
            CPPUNIT_LIBRARY
            NAMES cppunit
        )

        if(CPPUNIT_INCLUDE_DIR AND CPPUNIT_LIBRARY)
            add_library(CppUnit::CppUnit UNKNOWN IMPORTED)

            set_target_properties(
                CppUnit::CppUnit
                PROPERTIES
                    IMPORTED_LOCATION "${CPPUNIT_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${CPPUNIT_INCLUDE_DIR}"
            )

            set(STARGUS_CPPUNIT_TARGET CppUnit::CppUnit)
        endif()

        mark_as_advanced(
            CPPUNIT_INCLUDE_DIR
            CPPUNIT_LIBRARY
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Generated configuration
# ---------------------------------------------------------------------------

set(PACKAGE_DATA_DIR "${STARGUS_DATA_PATH}/")
set(PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/")

file(
    CONFIGURE
    OUTPUT "${PROJECT_BINARY_DIR}/config.h"
    CONTENT [=[
#pragma once

#define PACKAGE_DATA_DIR "@PACKAGE_DATA_DIR@"
#define PACKAGE_SOURCE_DIR "@PACKAGE_SOURCE_DIR@"
]=]
    @ONLY
)

# ---------------------------------------------------------------------------
# starformat
# ---------------------------------------------------------------------------

set(STARGUS_KAITAI_SOURCES
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
)

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

    ${STARGUS_KAITAI_SOURCES}

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
        "${PROJECT_BINARY_DIR}"
)

target_compile_definitions(
    starformat
    PUBLIC
        HAVE_CONFIG_H
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
        ${STARGUS_ICONV_TARGET}
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

if(TARGET ImageMagick::Magick++)
    target_compile_definitions(
        starformat
        PUBLIC
            HAVE_IMAGEMAGICKPP
    )

    target_link_libraries(
        starformat
        PRIVATE
            ImageMagick::Magick++
    )
endif()

if(TARGET PkgConfig::LOG4CXX)
    target_compile_definitions(
        starformat
        PUBLIC
            HAVE_LOG4CXX
    )

    target_link_libraries(
        starformat
        PUBLIC
            PkgConfig::LOG4CXX
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
# Startool 4
# ---------------------------------------------------------------------------

add_executable(
    startool4
    tools/startool/main.cpp
    tools/startool/StartoolApp.cpp
    tools/startool/SourceTypes.cpp
    tools/startool/LogicalSource.cpp
    tools/startool/SourceDetector.cpp
    tools/startool/SourceReaderFactory.cpp
    tools/startool/SourceReaderRegistry.cpp
    tools/startool/MpqArchive.cpp
    tools/startool/MpqSourceReader.cpp
    tools/startool/MpqVerifier.cpp
    tools/startool/LogicalSourceVerifier.cpp
    tools/startool/manifest/ManifestLoader.cpp
    tools/startool/pipeline/ImportPlanner.cpp
    tools/startool/pipeline/ImportExecutor.cpp
    tools/startool/converters/WavToOggConverter.cpp
    tools/startool/converters/PcxToPngConverter.cpp
    tools/startool/converters/TilesetToLuaConverter.cpp
    tools/startool/converters/TilesetToPngConverter.cpp
    tools/startool/converters/GrpToPngConverter.cpp
    tools/startool/formats/PcxDecoder.cpp
    tools/startool/formats/GrpDecoder.cpp
    tools/startool/palette/Pcx2DPaletteLoader.cpp
    tools/startool/palette/PcxPaletteLoader.cpp
    tools/startool/palette/WpePaletteLoader.cpp
)

target_include_directories(
    startool4
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
)

target_link_libraries(
    startool4
    PRIVATE
        StormLib::StormLib
        nlohmann_json::nlohmann_json
        Vorbis::Encoder
        PNG::PNG
)

# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------

add_executable(
    scdat2json
    tools/scdat2json/scdat2json.cpp
    tools/scdat2json/SCJsonExporter.cpp
    tools/scdat2json/UnitsJsonExporter.cpp
    tools/scdat2json/to_json.cpp
)

target_include_directories(
    scdat2json
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/scdat2json"
)

target_link_libraries(
    scdat2json
    PRIVATE
        starformat
)

install(
    TARGETS scdat2json
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

add_executable(
    grptool
    tools/grptool/grptool.cpp
)

target_link_libraries(
    grptool
    PRIVATE
        starformat
)

install(
    TARGETS grptool
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

add_executable(
    sauwetter
    tools/sauwetter/sauwetter.cpp
)

target_link_libraries(
    sauwetter
    PRIVATE
        starformat
)

install(
    TARGETS sauwetter
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

add_executable(
    iscript
    tools/iscript.c
)

add_executable(
    json_transition
    tools/json_transition.cpp
)

target_link_libraries(
    json_transition
    PRIVATE
        PNG::PNG
        nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------------------------
# Stargus launcher
# ---------------------------------------------------------------------------
if(Stratagus_FOUND)
    set(STARGUS_SOURCES
        src/stargus.cpp
    )

    if(WIN32)
        list(APPEND STARGUS_SOURCES packaging/windows/stargus.rc)
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

else()

    message(
        STATUS
        "Stratagus not found; Stargus launcher will not be built"
    )

endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(BUILD_TESTING)
    add_executable(
        LuaTest
        tests/lua/CreateUnitLuaTest.cpp
        tests/lua/LuaTest.cpp
    )

    target_include_directories(
        LuaTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/lua"
    )

    target_link_libraries(
        LuaTest
        PRIVATE
            starformat
    )

    target_compile_definitions(
        LuaTest
        PRIVATE
            STARGUS_TEST_OUTPUT_DIR="${PROJECT_BINARY_DIR}/tests/generated"
    )

    add_test(
        NAME LuaTest
        COMMAND LuaTest
    )

    set_tests_properties(
        LuaTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    if(STARGUS_CPPUNIT_TARGET)
        add_executable(
            StartoolUnitTest
            tests/module/TestHelpers.cpp
            tests/module/TestMain.cpp
            tests/module/StormTest.cpp
            tests/module/DataChunkTest.cpp
            tests/module/BreezeTest.cpp
            tests/module/PcxTest.cpp
            tests/module/TiledPaletteImageTest.cpp
            tests/module/luagenTest.cpp
        )

        target_include_directories(
            StartoolUnitTest
            PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/module"
        )

        target_link_libraries(
            StartoolUnitTest
            PRIVATE
                starformat
                ZLIB::ZLIB
                ${STARGUS_CPPUNIT_TARGET}
        )

        add_test(
            NAME StartoolUnitTest
            COMMAND StartoolUnitTest
        )

        set_tests_properties(
            StartoolUnitTest
            PROPERTIES
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )
    else()
        message(
            STATUS
            "CppUnit not found; StartoolUnitTest will not be built"
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------

set(STARGUS_WARNING_TARGETS
    starformat
    stargus
    startool
    scdat2json
    grptool
    sauwetter
    iscript
    json_transition
    LuaTest
    StartoolUnitTest
)

foreach(target IN LISTS STARGUS_WARNING_TARGETS)
    if(TARGET ${target})
        if(MSVC)
            target_compile_options(
                ${target}
                PRIVATE
                    /W4
            )
        else()
            target_compile_options(
                ${target}
                PRIVATE
                    -Wall
                    -Wextra
                    -Wpedantic
            )
        endif()
    endif()
endforeach()

# Kaitai sources are generated/vendored code. Keep normal project warnings
# enabled, but silence generated unused-parameter/local-variable noise.
if(MSVC)
    set_source_files_properties(
        ${STARGUS_KAITAI_SOURCES}
        PROPERTIES
            COMPILE_OPTIONS "/wd4100;/wd4189"
    )
else()
    set_source_files_properties(
        ${STARGUS_KAITAI_SOURCES}
        PROPERTIES
            COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-unused-but-set-variable"
    )
endif()

# ---------------------------------------------------------------------------
# UNIX desktop integration
# ---------------------------------------------------------------------------

if(NOT WIN32)
    configure_file(
        "${PROJECT_SOURCE_DIR}/packaging/linux/stargus.desktop.in"
        "${PROJECT_BINARY_DIR}/stargus.desktop"
        @ONLY
    )

    add_custom_command(
        OUTPUT "${PROJECT_BINARY_DIR}/stargus.png"
        COMMAND
            "${IMAGEMAGICK_CONVERT}"
            "${PROJECT_SOURCE_DIR}/packaging/windows/stargus.ico[0]"
            "${PROJECT_BINARY_DIR}/stargus.png"
        DEPENDS "${PROJECT_SOURCE_DIR}/packaging/windows/stargus.ico"
        COMMENT "Generating stargus.png"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${PROJECT_BINARY_DIR}/stargus.xpm"
        COMMAND
            "${IMAGEMAGICK_CONVERT}"
            "${PROJECT_SOURCE_DIR}/packaging/windows/stargus.ico[0]"
            -resize 32x32
            "${PROJECT_BINARY_DIR}/stargus.xpm"
        DEPENDS "${PROJECT_SOURCE_DIR}/packaging/windows/stargus.ico"
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
            contrib
            scripts
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

    file(
        COPY
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/stargus.ico"
            "${CMAKE_CURRENT_SOURCE_DIR}/mpqlist.txt"
        DESTINATION
            "${CMAKE_CURRENT_BINARY_DIR}"
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
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/stargus.nsi"
        COMMAND
            "${MAKENSIS_EXECUTABLE}"
            ${MAKENSIS_FLAGS_LIST}
            -DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/stargus.nsi"
        DEPENDS
            stargus
            startool
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/stargus.nsi"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            "${CMAKE_CURRENT_SOURCE_DIR}/mpqlist.txt"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/stargus.ico"
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
