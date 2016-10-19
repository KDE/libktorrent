#.rst:
# FindLibGMP
# ----------
#
# Try to find the GNU MP Library (libgmp).
#
# This will define the following variables:
#
# ``LibGMP_FOUND``
#     True if libgmp is available.
#
# ``LibGMP_VERSION``
#     The version of LibGMP
#
# ``LibGMP_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if
#     the target is not used for linking
#
# ``LibGMP_LIBRARIES``
#     This can be passed to target_link_libraries() instead of
#     the ``LibGMP::LibGMP`` target
#
# If ``LibGMP_FOUND`` is TRUE, the following imported target
# will be available:
#
# ``LibGMP::LibGMP``
#     The libgmp library
#
# Since 1.9.50.

#=============================================================================
# Copyright 2006 Laurent Montel, <montel@kde.org>
# Copyright 2016 Christophe Giboudeaux <cgiboudeaux@gmx.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

find_path(LibGMP_INCLUDE_DIRS NAMES gmp.h)

find_library(LibGMP_LIBRARIES NAMES gmp libgmp)

# Get version from gmp.h
if(LibGMP_INCLUDE_DIRS)
    file(STRINGS ${LibGMP_INCLUDE_DIRS}/gmp.h _GMP_H REGEX "^#define __GNU_MP_VERSION.*$")
    if(_GMP_H)
        string(REGEX REPLACE "^.*__GNU_MP_VERSION[ ]+([0-9]+).*$" "\\1" LibGMP_MAJOR_VERSION "${_GMP_H}")
        string(REGEX REPLACE "^.*__GNU_MP_VERSION_MINOR[ ]+([0-9]+).*$" "\\1" LibGMP_MINOR_VERSION "${_GMP_H}")
        string(REGEX REPLACE "^.*__GNU_MP_VERSION_PATCHLEVEL[ ]+([0-9]+).*$" "\\1" LibGMP_PATCH_VERSION "${_GMP_H}")

        set(LibGMP_VERSION "${LibGMP_MAJOR_VERSION}.${LibGMP_MINOR_VERSION}.${LibGMP_PATCH_VERSION}")
        unset(_GMP_H)
    else()
        # parsing gmp.h failed, try test code instead
        set(_gmp_version_source "
#include <stddef.h>
#include <stdio.h>
#include <gmp.h>
int main()
{
  printf(\"%d.%d.%d\",__GNU_MP_VERSION, __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL);
}
"       )
        set(_gmp_version_source_file ${CMAKE_BINARY_DIR}/CMakeTmp/cmake_gmp_version_check.cpp)
        file(WRITE "${_gmp_version_source_file}" "${_gmp_version_source}")
        try_run(_gmp_version_compile_result _gmp_version_run_result ${CMAKE_BINARY_DIR} ${_gmp_version_source_file}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${LibGMP_INCLUDE_DIRS}"
                RUN_OUTPUT_VARIABLE _gmp_version_output )

        set(LibGMP_VERSION "${_gmp_version_output}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGMP
    FOUND_VAR LibGMP_FOUND
    REQUIRED_VARS LibGMP_INCLUDE_DIRS LibGMP_LIBRARIES
    VERSION_VAR LibGMP_VERSION
)

if(LibGMP_FOUND AND NOT TARGET LibGMP::LibGMP)
    add_library(LibGMP::LibGMP UNKNOWN IMPORTED)
    set_target_properties(LibGMP::LibGMP PROPERTIES
        IMPORTED_LOCATION "${LibGMP_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibGMP_INCLUDE_DIRS}")
endif()

mark_as_advanced(LibGMP_INCLUDE_DIRS LibGMP_LIBRARIES)

include(FeatureSummary)
set_package_properties(LibGMP PROPERTIES
    URL "https://gmplib.org/"
    DESCRIPTION "A library for calculating huge numbers (integer and floating point)."
)
