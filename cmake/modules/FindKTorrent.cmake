# - Try to find the libktorrent library
# Once done this will define
#
#  LIBKTORRENT_FOUND - system has libktorrent
#  LIBKTORRENT_INCLUDE_DIR - the libktorrent include directory
#  LIBKTORRENT_LIBRARIES - Link these to use libktorrent

# Copyright (c) 2007 Joris Guisson <joris.guisson@gmail.com>
# Copyright (c) 2007 Charles Connell <charles@connells.org> (This was based upon FindKopete.cmake)
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LIBKTORRENT_INCLUDE_DIR AND LIBKTORRENT_LIBRARIES)

  # read from cache
  set(LIBKTORRENT_FOUND TRUE)

else(LIBKTORRENT_INCLUDE_DIR AND LIBKTORRENT_LIBRARIES)

  FIND_PATH(LIBKTORRENT_INCLUDE_DIR 
    NAMES
    ktorrent_export.h
    PATHS 
    ${KDE4_INCLUDE_DIR}
    ${INCLUDE_INSTALL_DIR}
    PATH_SUFFIXES
    libktorrent
    )
  
  FIND_LIBRARY(LIBKTORRENT_LIBRARIES 
    NAMES
    ktorrent
    PATHS
    ${KDE4_LIB_DIR}
    ${LIB_INSTALL_DIR}
    )
  if(LIBKTORRENT_INCLUDE_DIR AND LIBKTORRENT_LIBRARIES)
    set(LIBKTORRENT_FOUND TRUE)
  endif(LIBKTORRENT_INCLUDE_DIR AND LIBKTORRENT_LIBRARIES)

  if(MSVC)
    FIND_LIBRARY(LIBKTORRENT_LIBRARIES_DEBUG 
      NAMES
      ktorrentd
      PATHS
      ${KDE4_LIB_DIR}
      ${LIB_INSTALL_DIR}
      )
    if(NOT LIBKTORRENT_LIBRARIES_DEBUG)
      set(LIBKTORRENT_FOUND FALSE)
    endif(NOT LIBKTORRENT_LIBRARIES_DEBUG)
    
    if(MSVC_IDE)
      if( NOT LIBKTORRENT_LIBRARIES_DEBUG OR NOT LIBKTORRENT_LIBRARIES)
        message(FATAL_ERROR "\nCould NOT find the debug AND release version of the libktorrent library.\nYou need to have both to use MSVC projects.\nPlease build and install both libktorrent libraries first.\n")
      endif( NOT LIBKTORRENT_LIBRARIES_DEBUG OR NOT LIBKTORRENT_LIBRARIES)
    else(MSVC_IDE)
      string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
      if(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
        set(LIBKTORRENT_LIBRARIES ${LIBKTORRENT_LIBRARIES_DEBUG})
      else(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
        set(LIBKTORRENT_LIBRARIES ${LIBKTORRENT_LIBRARIES})
      endif(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
    endif(MSVC_IDE)
  endif(MSVC)

  if(LIBKTORRENT_FOUND)
    if(NOT LIBKTORRENT_FIND_QUIETLY)
      message(STATUS "Found libktorrent: ${LIBKTORRENT_LIBRARIES} ")
    endif(NOT LIBKTORRENT_FIND_QUIETLY)
  else(LIBKTORRENT_FOUND)
    if(LIBKTORRENT_FIND_REQUIRED)
      if(NOT LIBKTORRENT_INCLUDE_DIR)
	message(FATAL_ERROR "Could not find libktorrent includes.")
      endif(NOT LIBKTORRENT_INCLUDE_DIR)
      if(NOT LIBKTORRENT_LIBRARIES)
	message(FATAL_ERROR "Could not find libktorrent library.")
      endif(NOT LIBKTORRENT_LIBRARIES)
    else(LIBKTORRENT_FIND_REQUIRED)
      if(NOT LIBKTORRENT_INCLUDE_DIR)
        message(STATUS "Could not find libktorrent includes.")
      endif(NOT LIBKTORRENT_INCLUDE_DIR)
      if(NOT LIBKTORRENT_LIBRARIES)
        message(STATUS "Could not find libktorrent library.")
      endif(NOT LIBKTORRENT_LIBRARIES)
    endif(LIBKTORRENT_FIND_REQUIRED)
  endif(LIBKTORRENT_FOUND)

endif(LIBKTORRENT_INCLUDE_DIR AND LIBKTORRENT_LIBRARIES)
