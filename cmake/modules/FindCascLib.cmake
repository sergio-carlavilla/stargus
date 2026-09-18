# SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

find_path(
    CascLib_INCLUDE_DIR
    NAMES CascLib.h
)

find_library(
    CascLib_LIBRARY
    NAMES casc CascLib
)

find_package_handle_standard_args(
    CascLib
    REQUIRED_VARS
        CascLib_LIBRARY
        CascLib_INCLUDE_DIR
)

if(CascLib_FOUND AND NOT TARGET CascLib::CascLib)
    add_library(CascLib::CascLib UNKNOWN IMPORTED)

    set_target_properties(
        CascLib::CascLib
        PROPERTIES
            IMPORTED_LOCATION "${CascLib_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CascLib_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(
    CascLib_INCLUDE_DIR
    CascLib_LIBRARY
)
