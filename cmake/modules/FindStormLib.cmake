# SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

find_path(
    StormLib_INCLUDE_DIR
    NAMES StormLib.h
)

find_library(
    StormLib_LIBRARY
    NAMES storm StormLib
)

find_package_handle_standard_args(
    StormLib
    REQUIRED_VARS
        StormLib_LIBRARY
        StormLib_INCLUDE_DIR
)

if(StormLib_FOUND AND NOT TARGET StormLib::StormLib)
    add_library(StormLib::StormLib UNKNOWN IMPORTED)

    set_target_properties(
        StormLib::StormLib
        PROPERTIES
            IMPORTED_LOCATION "${StormLib_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${StormLib_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(
    StormLib_INCLUDE_DIR
    StormLib_LIBRARY
)
