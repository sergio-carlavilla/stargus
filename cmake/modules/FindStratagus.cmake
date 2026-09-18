# SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

set(
    Stratagus_ROOT
    ""
    CACHE PATH
    "Installation prefix or root directory containing Stratagus"
)

find_path(
    Stratagus_INCLUDE_DIR
    NAMES stratagus-game-launcher.h
    HINTS
        "${Stratagus_ROOT}"
    PATH_SUFFIXES
        include
        include/stratagus
        gameheaders
)

find_program(
    Stratagus_EXECUTABLE
    NAMES
        stratagus
        stratagus-dbg
    HINTS
        "${Stratagus_ROOT}"
    PATH_SUFFIXES
        bin
        games
)

find_package_handle_standard_args(
    Stratagus
    REQUIRED_VARS
        Stratagus_EXECUTABLE
        Stratagus_INCLUDE_DIR
)

if(Stratagus_FOUND)
    if(NOT TARGET Stratagus::GameHeaders)
        add_library(Stratagus::GameHeaders INTERFACE IMPORTED)

        set_target_properties(
            Stratagus::GameHeaders
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES
                    "${Stratagus_INCLUDE_DIR}"
        )
    endif()

    if(NOT TARGET Stratagus::stratagus)
        add_executable(Stratagus::stratagus IMPORTED)

        set_target_properties(
            Stratagus::stratagus
            PROPERTIES
                IMPORTED_LOCATION
                    "${Stratagus_EXECUTABLE}"
        )
    endif()
endif()

mark_as_advanced(
    Stratagus_INCLUDE_DIR
    Stratagus_EXECUTABLE
)
