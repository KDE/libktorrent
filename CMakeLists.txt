project(LIBKTORRENT)

cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules;${CMAKE_MODULE_PATH}")
find_package(LibGcrypt REQUIRED)

# set some default settings
enable_testing()
set(BUILD_TESTING ON)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

#
# checks for functions and stuff
#
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckTypeSize) #XFS

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMMarkAsTest)
include(ECMSetupVersion)
include(ECMPackageConfigHelpers)

set(LIBKTORRENT_VERSION "1.9.50")

ecm_setup_version(${LIBKTORRENT_VERSION} VARIABLE_PREFIX LIBKTORRENT
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/libktorrent_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LibKTorrentConfigVersion.cmake"
                        SOVERSION 1)
find_package(GMP REQUIRED)
find_package(Boost REQUIRED)
find_package(Qca-qt5 REQUIRED)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
  Core
  Gui
  Widgets
  Network
  Xml
  Test
)

# Load the frameworks we need
find_package(KF5 REQUIRED
  Archive
  I18n
  KIO
  Solid
)

kde_enable_exceptions()
add_definitions(-DQT_USE_QSTRINGBUILDER)
include_directories( ${LIBKTORRENT_BINARY_DIR}	${LIBKTORRENT_SOURCE_DIR} ${Qt5Core_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${QCA-qt5_INCLUDE_DIR} ${Boost_INCLUDE_DIR}) #${GMP_INCLUDE_DIR}

#XFS prealloc
CHECK_INCLUDE_FILES(xfs/xfs.h HAVE_XFS_XFS_H)
IF(HAVE_XFS_XFS_H)
	SET(CMAKE_EXTRA_INCLUDE_FILES xfs/xfs.h)
	CHECK_TYPE_SIZE(__u64 HAVE___U64)
	CHECK_TYPE_SIZE(__s64 HAVE___S64)
	SET(CMAKE_EXTRA_INCLUDE_FILES)
ENDIF(HAVE_XFS_XFS_H)

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
add_subdirectory(testlib)
add_subdirectory(examples)
add_subdirectory(doc)

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(INCLUDE_INSTALL_DIR "include/libktorrent/")

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/LibKTorrent")

ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/LibKTorrentConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/LibKTorrentConfig.cmake"
                              INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
                              PATH_VARS INCLUDE_INSTALL_DIR
                              )

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/LibKTorrentConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/LibKTorrentConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel )

install(EXPORT LibKTorrentTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE LibKTorrentTargets.cmake )


feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

