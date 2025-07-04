cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

cmake_policy(SET CMP0057 NEW)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")


set(Boost_MIN_VERSION "1.71.0")
set(LibGMP_MIN_VERSION "6.0.0")
set(LibGcrypt_MIN_VERSION "1.4.5")
set(OpenSSL_MIN_VERSION "3.0.0")
set(KF_MIN_VERSION "5.240.0")
set(QT_MIN_VERSION "6.5.0")

set (SOVERSION 6)

project(LIBKTORRENT VERSION ${RELEASE_SERVICE_VERSION})

# set some default settings
enable_testing()

set(CRYPTO_BACKENDS "Auto" "OpenSSL" "LibGcrypt")
set(USE_CRYPTO_BACKEND "Auto" CACHE STRING "Cryptography backend to use in libktorrent")
set_property(CACHE USE_CRYPTO_BACKEND PROPERTY STRINGS ${CRYPTO_BACKENDS})
if (NOT USE_CRYPTO_BACKEND IN_LIST CRYPTO_BACKENDS)
    message(FATAL_ERROR "USE_CRYPTO_BACKEND must be one of: ${CRYPTO_BACKENDS}")
endif()

find_package(ECM ${KF_MIN_VERSION} NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

#
# checks for functions and stuff
#
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckTypeSize) #XFS

include(KDEInstallDirs)
include(KDECMakeSettings)
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMMarkAsTest)
include(ECMSetupVersion)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_REQUIRED_VERSION "6.5.0")
set(KF_MIN_VERSION "5.240.0")
ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX LIBKTORRENT
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/libktorrent_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KTorrent6ConfigVersion.cmake"
                        SOVERSION "${SOVERSION}")

find_package(Qt6 "${QT_MIN_VERSION}" CONFIG REQUIRED COMPONENTS
    Core
    Network
    Xml
    Test
)

# Load the frameworks we need
find_package(KF6 "${KF_MIN_VERSION}" REQUIRED COMPONENTS
    Archive
    Crash
    I18n
    KIO
    Solid
)

find_package(Boost "${Boost_MIN_VERSION}" NO_MODULE REQUIRED)
find_package(LibGMP "${LibGMP_MIN_VERSION}" REQUIRED)

if (USE_CRYPTO_BACKEND STREQUAL "Auto" OR USE_CRYPTO_BACKEND STREQUAL "OpenSSL")
    find_package(OpenSSL "${OpenSSL_MIN_VERSION}")
endif()
if (NOT OPENSSL_FOUND AND (USE_CRYPTO_BACKEND STREQUAL "Auto" OR USE_CRYPTO_BACKEND STREQUAL "LibGcrypt"))
    find_package(LibGcrypt "${LibGcrypt_MIN_VERSION}")
endif()
if (OPENSSL_FOUND)
    set(USE_OPENSSL 1)
    set(USE_LIBGCRYPT 0)
elseif (LibGcrypt_FOUND)
    set(USE_LIBGCRYPT 1)
    set(USE_OPENSSL 0)
else()
    message(FATAL_ERROR "No cryptography library found, USE_CRYPTO_BACKEND is ${USE_CRYPTO_BACKEND}")
endif()

kde_enable_exceptions()
add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_CAST_FROM_ASCII
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_NO_CAST_TO_BYTEARRAY
  -DQT_NO_KEYWORDS
  -DQT_USE_FAST_OPERATOR_PLUS
)
add_definitions(-DTRANSLATION_DOMAIN="libktorrent6")

option(WITH_XFS "Support for XFS specific disk preallocation" ON)
if(WITH_XFS)
    #XFS prealloc
    check_include_files(xfs/xfs.h HAVE_XFS_XFS_H)
    if(HAVE_XFS_XFS_H)
        set(CMAKE_EXTRA_INCLUDE_FILES xfs/xfs.h)
        check_type_size(__u64 HAVE___U64)
        check_type_size(__s64 HAVE___S64)
        set(CMAKE_EXTRA_INCLUDE_FILES)
        message(STATUS "xfsprogs found, building with XFS specific disk preallocation support")
    else()
        message(STATUS "xfsprogs NOT found, XFS specific disk preallocation unavailable")
    endif()
endif()

# check for 64 bit file I/O functions
check_function_exists(fopen64 HAVE_FOPEN64)
check_function_exists(fseeko64 HAVE_FSEEKO64)
check_function_exists(fseeko HAVE_FSEEKO)
check_function_exists(ftello64 HAVE_FTELLO64)
check_function_exists(ftello HAVE_FTELLO)
check_function_exists(fstat64 HAVE_FSTAT64)
check_function_exists(stat64 HAVE_STAT64)
check_function_exists(ftruncate64 HAVE_FTRUNCATE64)
check_function_exists(lseek64 HAVE_LSEEK64)
check_function_exists(mmap64 HAVE_MMAP64)
check_function_exists(munmap64 HAVE_MUNMAP64)
check_function_exists(posix_fallocate64 HAVE_POSIX_FALLOCATE64)
check_function_exists(posix_fallocate HAVE_POSIX_FALLOCATE)
check_function_exists(fallocate HAVE_FALLOCATE)
check_function_exists(statvfs HAVE_STATVFS)
check_function_exists(statvfs64 HAVE_STATVFS64)

add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(testlib)
endif()
add_subdirectory(examples)
add_subdirectory(doc)

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KTorrent6")

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/KTorrent6Config.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/KTorrent6Config.cmake"
                              INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
                              PATH_VARS KDE_INSTALL_INCLUDEDIR
                              )

# Install the CMake modules needed to use libktorrent
add_subdirectory(cmake)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/KTorrent6Config.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/KTorrent6ConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)

install(EXPORT KTorrent6Targets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KTorrent6Targets.cmake)

ki18n_install(po)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

