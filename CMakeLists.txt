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

find_package(GMP REQUIRED)
find_package(Boost REQUIRED)
find_package(Qca-qt5 REQUIRED)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
  Core
  Gui
  Network
  Test
  Widgets
)

# Load the frameworks we need
find_package(KF5 REQUIRED
  Archive
  I18n
  KIO
  KDELibs4Support
)

kde_enable_exceptions()
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
include_directories( ${LIBKTORRENT_BINARY_DIR}	${LIBKTORRENT_SOURCE_DIR} ${Qt5Core_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${QCA-qt5_INCLUDE_DIR} ${Boost_INCLUDE_DIR}) #${GMP_INCLUDE_DIR}

set(LIBKTORRENT_VERSION "1.1dev")

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
#add_subdirectory(testlib)
add_subdirectory(examples)
add_subdirectory(cmake)
add_subdirectory(doc)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

