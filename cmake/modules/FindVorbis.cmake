# SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only

include(FindPackageHandleStandardArgs)

find_path(
    VORBIS_INCLUDE_DIR
    NAMES
        vorbis/vorbisenc.h
)

find_library(
    VORBIS_LIBRARY
    NAMES
        vorbis
)

find_library(
    VORBISENC_LIBRARY
    NAMES
        vorbisenc
)

find_library(
    OGG_LIBRARY
    NAMES
        ogg
)

find_package_handle_standard_args(
    Vorbis
    REQUIRED_VARS
        VORBIS_INCLUDE_DIR
        VORBIS_LIBRARY
        VORBISENC_LIBRARY
        OGG_LIBRARY
)

if(Vorbis_FOUND AND NOT TARGET Vorbis::Encoder)
    add_library(
        Vorbis::Encoder
        INTERFACE
        IMPORTED
    )

    set_target_properties(
        Vorbis::Encoder
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
                "${VORBIS_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES
                "${VORBISENC_LIBRARY};${VORBIS_LIBRARY};${OGG_LIBRARY}"
    )
endif()

mark_as_advanced(
    VORBIS_INCLUDE_DIR
    VORBIS_LIBRARY
    VORBISENC_LIBRARY
    OGG_LIBRARY
)
