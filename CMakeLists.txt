# SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only

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

option(WITH_CASCLIB "Compile Startool4 with CascLib support" ON)

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

if(NOT WIN32)
    find_program(
        IMAGEMAGICK_CONVERT
        NAMES magick convert
        REQUIRED
    )
endif()

# ---------------------------------------------------------------------------
# Startool 4
# ---------------------------------------------------------------------------

file(RELATIVE_PATH STARTOOL4_RELATIVE_ASSETS "${GAMEDIRABS}" "${STARGUS_DATA_PATH}")
file(
    CONFIGURE
    OUTPUT "${PROJECT_BINARY_DIR}/LauncherPaths.h"
    CONTENT [=[
#pragma once
#define STARTOOL4_SOURCE_ASSETS "@PROJECT_SOURCE_DIR@"
#define STARTOOL4_INSTALLED_ASSETS "@STARGUS_DATA_PATH@"
#define STARTOOL4_RELATIVE_ASSETS "@STARTOOL4_RELATIVE_ASSETS@"
]=]
    @ONLY
)

add_executable(
    startool4
    tools/startool/main.cpp
    tools/startool/StartoolApp.cpp
    tools/startool/LauncherImport.cpp
    tools/startool/SourceTypes.cpp
    tools/startool/GameLocale.cpp
    tools/startool/GameLocaleDetector.cpp
    tools/startool/LogicalSource.cpp
    tools/startool/SourceDetector.cpp
    tools/startool/SourceReaderFactory.cpp
    tools/startool/FilesystemSourceReader.cpp
    tools/startool/SourceReaderRegistry.cpp
    tools/startool/OverlaySourceReader.cpp
    tools/startool/MpqArchive.cpp
    tools/startool/MpqSourceReader.cpp
    tools/startool/MpqVerifier.cpp
    tools/startool/LogicalSourceVerifier.cpp
    tools/startool/manifest/ManifestLoader.cpp
    tools/startool/pipeline/ImportPlanner.cpp
    tools/startool/pipeline/ImportExecutor.cpp
    tools/startool/converters/WavToOggConverter.cpp
    tools/startool/converters/DdsToPngConverter.cpp
    tools/startool/converters/PcxToPngConverter.cpp
    tools/startool/converters/PanelToPngConverter.cpp
    tools/startool/converters/UiConsoleConverter.cpp
    tools/startool/converters/PortraitAssetResolver.cpp
    tools/startool/converters/PortraitLuaWriter.cpp
    tools/startool/converters/FontToPngConverter.cpp
    tools/startool/converters/SmkToMngConverter.cpp
    tools/startool/converters/SmkToOgvConverter.cpp
    tools/startool/converters/WebmToMngConverter.cpp
    tools/startool/converters/WebmToOgvConverter.cpp
    tools/startool/converters/TilesetToLuaConverter.cpp
    tools/startool/converters/TilesetToPngConverter.cpp
    tools/startool/converters/GrpFrameToPngConverter.cpp
    tools/startool/converters/GrpFramesToPngConverter.cpp
    tools/startool/converters/GrpToPngConverter.cpp
    tools/startool/converters/ImageAssetResolver.cpp
    tools/startool/converters/ImageLuaWriter.cpp
    tools/startool/converters/ImagesLuaIndexWriter.cpp
    tools/startool/converters/UnitDataResolver.cpp
    tools/startool/converters/UnitLuaWriter.cpp
    tools/startool/converters/UnitsLuaWriter.cpp
    tools/startool/converters/ChkLuaWriter.cpp
    tools/startool/converters/TextToUtf8Converter.cpp
    tools/startool/formats/DdsDecoder.cpp
    tools/startool/formats/PcxDecoder.cpp
    tools/startool/formats/FontDecoder.cpp
    tools/startool/formats/GrpDecoder.cpp
    tools/startool/formats/ImagesDatDecoder.cpp
    tools/startool/formats/TblDecoder.cpp
    tools/startool/formats/UnitsDatDecoder.cpp
    tools/startool/formats/FlingyDatDecoder.cpp
    tools/startool/formats/SpritesDatDecoder.cpp
    tools/startool/formats/PortDataDecoder.cpp
    tools/startool/formats/SfxDataDecoder.cpp
    tools/startool/formats/ScmScenarioExtractor.cpp
    tools/startool/formats/ChkDecoder.cpp
    tools/startool/palette/Pcx2DPaletteLoader.cpp
    tools/startool/palette/PcxPaletteLoader.cpp
    tools/startool/palette/WpePaletteLoader.cpp
)

target_include_directories(
    startool4
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
        "${PROJECT_BINARY_DIR}"
)

if(WITH_CASCLIB)
    target_sources(
        startool4
        PRIVATE
            tools/startool/CascArchive.cpp
            tools/startool/CascSourceReader.cpp
            tools/startool/CascVerifier.cpp
    )

    target_compile_definitions(
        startool4
        PRIVATE
            HAVE_CASC
    )

    target_link_libraries(
        startool4
        PRIVATE
            CascLib::CascLib
    )
endif()

target_link_libraries(
    startool4
    PRIVATE
        StormLib::StormLib
        nlohmann_json::nlohmann_json
        Vorbis::Encoder
        PNG::PNG
        ZLIB::ZLIB
)

# Keep the importer next to the launcher for prefixes outside PATH.
install(TARGETS startool4 RUNTIME DESTINATION "${STARGUS_GAME_BINDIR}" COMPONENT StargusRuntime)
install(DIRECTORY manifests DESTINATION "${STARGUS_DATA_DIR}" COMPONENT StargusRuntime)
install(DIRECTORY contrib scripts DESTINATION "${STARGUS_DATA_DIR}" COMPONENT StargusRuntime)

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

    add_dependencies(stargus startool4)

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
        COMPONENT StargusRuntime
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
        StartoolUnitLuaWriterTest
        tests/startool/UnitLuaWriterTest.cpp
        tools/startool/converters/UnitLuaWriter.cpp
    )

    target_include_directories(
        StartoolUnitLuaWriterTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolUnitLuaWriterTest
        COMMAND StartoolUnitLuaWriterTest
    )

    set_tests_properties(
        StartoolUnitLuaWriterTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;unit"
    )

    add_executable(
        StartoolOverlaySourceReaderTest
        tests/startool/OverlaySourceReaderTest.cpp
        tools/startool/OverlaySourceReader.cpp
    )

    target_include_directories(
        StartoolOverlaySourceReaderTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolOverlaySourceReaderTest
        COMMAND StartoolOverlaySourceReaderTest
    )

    set_tests_properties(
        StartoolOverlaySourceReaderTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;source;broodwar"
    )

    add_executable(
        StartoolFilesystemSourceReaderTest
        tests/startool/FilesystemSourceReaderTest.cpp
        tools/startool/FilesystemSourceReader.cpp
    )

    target_include_directories(
        StartoolFilesystemSourceReaderTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolFilesystemSourceReaderTest
        COMMAND StartoolFilesystemSourceReaderTest
    )

    set_tests_properties(
        StartoolFilesystemSourceReaderTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;source;casc"
    )

    add_executable(
        StartoolPortraitImportTest
        tests/startool/PortraitImportTest.cpp
    )

    target_link_libraries(
        StartoolPortraitImportTest
        PRIVATE
            StormLib::StormLib
    )

    add_dependencies(
        StartoolPortraitImportTest
        startool4
    )

    add_test(
        NAME StartoolPortraitImportTest
        COMMAND
            StartoolPortraitImportTest
            $<TARGET_FILE:startool4>
            "${PROJECT_SOURCE_DIR}/manifests/classic/portraits.json"
    )

    set_tests_properties(
        StartoolPortraitImportTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            SKIP_RETURN_CODE 77
            LABELS "startool;portrait;casc;integration"
    )

    add_executable(
        StartoolChkDecoderTest
        tests/startool/ChkDecoderTest.cpp
        tools/startool/formats/ChkDecoder.cpp
    )

    target_include_directories(
        StartoolChkDecoderTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolChkDecoderTest
        COMMAND StartoolChkDecoderTest
    )

    set_tests_properties(
        StartoolChkDecoderTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;chk;unit"
    )

    add_executable(
        StartoolChkLuaWriterTest
        tests/startool/ChkLuaWriterTest.cpp
        tools/startool/converters/ChkLuaWriter.cpp
    )

    target_include_directories(
        StartoolChkLuaWriterTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolChkLuaWriterTest
        COMMAND StartoolChkLuaWriterTest
    )

    set_tests_properties(
        StartoolChkLuaWriterTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;chk;unit"
    )

    add_executable(
        StartoolManifestValidationTest
        tests/startool/ManifestValidationTest.cpp
    )

    target_link_libraries(
        StartoolManifestValidationTest
        PRIVATE
            nlohmann_json::nlohmann_json
    )

    target_compile_definitions(
        StartoolManifestValidationTest
        PRIVATE
            STARGUS_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    )

    add_test(
        NAME StartoolManifestValidationTest
        COMMAND StartoolManifestValidationTest
    )

    set_tests_properties(
        StartoolManifestValidationTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;manifest;regression"
    )

    add_executable(
        StartoolDdsDecoderTest
        tests/startool/DdsDecoderTest.cpp
        tools/startool/formats/DdsDecoder.cpp
        tools/startool/converters/DdsToPngConverter.cpp
    )

    target_include_directories(
        StartoolDdsDecoderTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    target_link_libraries(
        StartoolDdsDecoderTest
        PRIVATE
            PNG::PNG
    )

    add_test(
        NAME StartoolDdsDecoderTest
        COMMAND StartoolDdsDecoderTest
    )

    set_tests_properties(
        StartoolDdsDecoderTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;image;unit"
    )

    add_executable(
        StartoolTblDecoderTest
        tests/startool/TblDecoderTest.cpp
        tools/startool/formats/TblDecoder.cpp
    )

    target_include_directories(
        StartoolTblDecoderTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolTblDecoderTest
        COMMAND StartoolTblDecoderTest
    )

    set_tests_properties(
        StartoolTblDecoderTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;image;unit"
    )

    add_executable(
        StartoolMpqSourceTest
        tests/startool/MpqSourceTest.cpp
        tools/startool/SourceDetector.cpp
        tools/startool/MpqVerifier.cpp
        tools/startool/MpqArchive.cpp
        tools/startool/MpqSourceReader.cpp
    )

    target_include_directories(StartoolMpqSourceTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool")
    target_link_libraries(StartoolMpqSourceTest PRIVATE StormLib::StormLib)
    add_test(NAME StartoolMpqSourceTest COMMAND StartoolMpqSourceTest)
    set_tests_properties(StartoolMpqSourceTest PROPERTIES LABELS "startool;mpq;unit")

    find_package(Python3 COMPONENTS Interpreter QUIET)

    add_executable(StartoolUnitReadySoundTest
        tests/startool/UnitReadySoundTest.cpp
        tools/startool/converters/UnitDataResolver.cpp
        tools/startool/converters/UnitLuaWriter.cpp
        tools/startool/converters/UnitsLuaWriter.cpp
        tools/startool/FilesystemSourceReader.cpp
        tools/startool/formats/UnitsDatDecoder.cpp
        tools/startool/formats/FlingyDatDecoder.cpp
        tools/startool/formats/SpritesDatDecoder.cpp
        tools/startool/formats/ImagesDatDecoder.cpp
        tools/startool/formats/PortDataDecoder.cpp
        tools/startool/formats/SfxDataDecoder.cpp
        tools/startool/formats/TblDecoder.cpp
    )
    target_include_directories(StartoolUnitReadySoundTest PRIVATE "${PROJECT_SOURCE_DIR}/tools/startool")
    target_link_libraries(StartoolUnitReadySoundTest PRIVATE StormLib::StormLib)
    add_test(NAME StartoolUnitReadySoundTest COMMAND StartoolUnitReadySoundTest)
    set_tests_properties(StartoolUnitReadySoundTest PROPERTIES LABELS "startool;unit;audio")

    if(Python3_Interpreter_FOUND)
        add_test(NAME StartoolUnitReadySoundCliTest
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/startool/UnitReadySoundCliTest.py"
                $<TARGET_FILE:startool4> $<TARGET_FILE:StartoolUnitReadySoundTest>)
        set_tests_properties(StartoolUnitReadySoundCliTest PROPERTIES LABELS "startool;audio;integration")
        add_test(
            NAME StartoolMpqCliTest
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/startool/MpqCliTest.py"
                $<TARGET_FILE:startool4>
                $<TARGET_FILE:StartoolMpqSourceTest>
        )
        set_tests_properties(StartoolMpqCliTest PROPERTIES LABELS "startool;mpq;integration")

        add_test(
            NAME StartoolLauncherCliTest
            COMMAND "${Python3_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/tests/startool/LauncherCliTest.py"
                $<TARGET_FILE:startool4> $<TARGET_FILE:StartoolMpqSourceTest>
        )
        set_tests_properties(StartoolLauncherCliTest PROPERTIES LABELS "startool;launcher;integration")

        if(Stratagus_FOUND AND UNIX AND NOT APPLE)
            add_executable(StargusLauncherEngineStub tests/startool/LauncherEngineStub.cpp)
            target_link_libraries(StargusLauncherEngineStub PRIVATE nlohmann_json::nlohmann_json)
            add_executable(StargusLauncherTestDriver src/stargus.cpp)
            target_link_libraries(StargusLauncherTestDriver PRIVATE Stratagus::GameHeaders)
            target_compile_definitions(StargusLauncherTestDriver PRIVATE
                SOURCE_DIR="${PROJECT_SOURCE_DIR}/"
                DATA_PATH="${PROJECT_BINARY_DIR}/tests/unused-launcher-data"
                SCRIPTS_PATH="${PROJECT_SOURCE_DIR}"
                STRATAGUS_BIN="$<TARGET_FILE:StargusLauncherEngineStub>"
            )
            add_dependencies(StargusLauncherTestDriver startool4 StargusLauncherEngineStub)
            add_test(
                NAME StargusLauncherSmokeTest
                COMMAND "${Python3_EXECUTABLE}"
                    "${PROJECT_SOURCE_DIR}/tests/startool/LauncherCliTest.py"
                    $<TARGET_FILE:startool4> $<TARGET_FILE:StartoolMpqSourceTest>
                    $<TARGET_FILE:StargusLauncherTestDriver>
            )
            set_tests_properties(StargusLauncherSmokeTest PROPERTIES LABELS "startool;launcher;integration")
        endif()
    endif()

    add_executable(
        StartoolUnitsDatDecoderTest
        tests/startool/UnitsDatDecoderTest.cpp
        tools/startool/formats/UnitsDatDecoder.cpp
    )

    target_include_directories(
        StartoolUnitsDatDecoderTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolUnitsDatDecoderTest
        COMMAND StartoolUnitsDatDecoderTest
    )

    set_tests_properties(
        StartoolUnitsDatDecoderTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;unit"
    )

    add_executable(
        StartoolImagesDatDecoderTest
        tests/startool/ImagesDatDecoderTest.cpp
        tools/startool/formats/ImagesDatDecoder.cpp
    )

    target_include_directories(
        StartoolImagesDatDecoderTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolImagesDatDecoderTest
        COMMAND StartoolImagesDatDecoderTest
    )

    set_tests_properties(
        StartoolImagesDatDecoderTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;image;unit"
    )

    add_executable(
        StartoolImageAssetResolverTest
        tests/startool/ImageAssetResolverTest.cpp
        tools/startool/converters/ImageAssetResolver.cpp
        tools/startool/formats/ImagesDatDecoder.cpp
        tools/startool/formats/TblDecoder.cpp
    )

    target_include_directories(
        StartoolImageAssetResolverTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolImageAssetResolverTest
        COMMAND StartoolImageAssetResolverTest
    )

    set_tests_properties(
        StartoolImageAssetResolverTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;image;regression"
    )

    add_executable(
        StartoolImageLuaWriterTest
        tests/startool/ImageLuaWriterTest.cpp
        tools/startool/converters/ImageLuaWriter.cpp
    )

    target_include_directories(
        StartoolImageLuaWriterTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolImageLuaWriterTest
        COMMAND StartoolImageLuaWriterTest
    )

    set_tests_properties(
        StartoolImageLuaWriterTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;image;regression"
    )

    add_executable(
        StartoolPortraitLuaWriterTest
        tests/startool/PortraitLuaWriterTest.cpp
        tools/startool/converters/PortraitLuaWriter.cpp
    )

    target_include_directories(
        StartoolPortraitLuaWriterTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/startool"
    )

    add_test(
        NAME StartoolPortraitLuaWriterTest
        COMMAND StartoolPortraitLuaWriterTest
    )

    set_tests_properties(
        StartoolPortraitLuaWriterTest
        PROPERTIES
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            LABELS "startool;portrait;regression"
    )

endif()

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------

set(STARGUS_WARNING_TARGETS
    stargus
    startool4
    StartoolUnitLuaWriterTest
    StartoolOverlaySourceReaderTest
    StartoolFilesystemSourceReaderTest
    StartoolPortraitImportTest
    StartoolChkDecoderTest
    StartoolChkLuaWriterTest
    StartoolManifestValidationTest
    StartoolDdsDecoderTest
    StartoolTblDecoderTest
    StartoolImagesDatDecoderTest
    StartoolUnitsDatDecoderTest
    StartoolUnitReadySoundTest
    StartoolMpqSourceTest
    StartoolImageAssetResolverTest
    StartoolImageLuaWriterTest
    StartoolPortraitLuaWriterTest
    StargusLauncherEngineStub
    StargusLauncherTestDriver
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
            startool4
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/stargus.nsi"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
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
