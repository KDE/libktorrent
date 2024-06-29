#.rst:
# FindLibGcrypt
# -------------
#
# Try to find libgcrypt.
#
# This will define the following variables:
#
# ``LibGcrypt_FOUND``
#     True if libgcrypt is available.
#
# ``LibGcrypt_VERSION``
#     The version of LibGcrypt
#
# ``LibGcrypt_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if
#     the target is not used for linking
#
# ``LibGcrypt_LIBRARIES``
#     This can be passed to target_link_libraries() instead of
#     the ``LibGcrypt::LibGcrypt`` target
#
# If ``LibGcrypt_FOUND`` is TRUE, the following imported target
# will be available:
#
# ``LibGcrypt::LibGcrypt``
#     The libgcrypt library
#
# Since 1.9.50.

#=============================================================================
# This was based upon FindKopete.cmake:
# SPDX-FileCopyrightText: 2007 Charles Connell <charles@connells.org>
#
# SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>
# SPDX-FileCopyrightText: 2014 Nicolás Alvarez <nicolas.alvarez@gmail.com>
# SPDX-FileCopyrightText: 2016 Christophe Giboudeaux <cgiboudeaux@gmx.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#=============================================================================

find_program(LIBGCRYPTCONFIG_SCRIPT NAMES libgcrypt-config)
if(LIBGCRYPTCONFIG_SCRIPT)
    execute_process(
        COMMAND "${LIBGCRYPTCONFIG_SCRIPT}" --prefix
        RESULT_VARIABLE CONFIGSCRIPT_RESULT
        OUTPUT_VARIABLE PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (CONFIGSCRIPT_RESULT EQUAL 0)
        set(LIBGCRYPT_LIB_HINT "${PREFIX}/lib")
        set(LIBGCRYPT_INCLUDE_HINT "${PREFIX}/include")
    endif()
endif()

find_library(LibGcrypt_LIBRARIES
    NAMES gcrypt
    HINTS ${LIBGCRYPT_LIB_HINT}
)
find_path(LibGcrypt_INCLUDE_DIRS
    NAMES gcrypt.h
    HINTS ${LIBGCRYPT_INCLUDE_HINT}
)

if(LibGcrypt_INCLUDE_DIRS)
    file(STRINGS ${LibGcrypt_INCLUDE_DIRS}/gcrypt.h GCRYPT_H REGEX "^#define GCRYPT_VERSION ")
    string(REGEX REPLACE "^#define GCRYPT_VERSION \"(.*)\".*$" "\\1" LibGcrypt_VERSION "${GCRYPT_H}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGcrypt
    FOUND_VAR LibGcrypt_FOUND
    REQUIRED_VARS LibGcrypt_INCLUDE_DIRS LibGcrypt_LIBRARIES
    VERSION_VAR LibGcrypt_VERSION
)

if(LibGcrypt_FOUND AND NOT TARGET LibGcrypt::LibGcrypt)
    add_library(LibGcrypt::LibGcrypt UNKNOWN IMPORTED)
    set_target_properties(LibGcrypt::LibGcrypt PROPERTIES
        IMPORTED_LOCATION "${LibGcrypt_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibGcrypt_INCLUDE_DIRS}")
endif()

mark_as_advanced(LibGcrypt_INCLUDE_DIRS LibGcrypt_LIBRARIES)

include(FeatureSummary)
set_package_properties(LibGcrypt PROPERTIES
    URL "http://directory.fsf.org/wiki/Libgcrypt"
    DESCRIPTION "General purpose crypto library based on the code used in GnuPG."
)
