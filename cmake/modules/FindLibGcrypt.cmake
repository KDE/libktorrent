# - Try to find the libgcrypt library
# Once done this will define
#
#  LIBGCRYPT_FOUND - system has libgcrypt
#  LIBGCRYPT_INCLUDE_DIR - the libgcrypt include directory
#  LIBGCRYPT_LIBRARIES - Link these to use libgcrypt

# Copyright (c) 2010 Joris Guisson <joris.guisson@gmail.com>
# Copyright (c) 2007 Charles Connell <charles@connells.org> (This was based upon FindKopete.cmake)
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LIBGCRYPT_INCLUDE_DIR AND LIBGCRYPT_LIBRARIES)
	
# read from cache
set(LIBGCRYPT_FOUND TRUE)

else(LIBGCRYPT_INCLUDE_DIR AND LIBGCRYPT_LIBRARIES)
	
	FIND_PATH(LIBGCRYPT_INCLUDE_DIR 
		NAMES
		gcrypt.h
		PATHS 
		${KDE4_INCLUDE_DIR}
		${INCLUDE_INSTALL_DIR}
		PATH_SUFFIXES
		libgcrypt
	)
	
	FIND_LIBRARY(LIBGCRYPT_LIBRARIES 
		NAMES
		gcrypt
		PATHS
		${KDE4_LIB_DIR}
		${LIB_INSTALL_DIR}
	)
	
	if(LIBGCRYPT_INCLUDE_DIR AND LIBGCRYPT_LIBRARIES)
		set(LIBGCRYPT_FOUND TRUE)
	endif(LIBGCRYPT_INCLUDE_DIR AND LIBGCRYPT_LIBRARIES)
		
	if(MSVC)
		FIND_LIBRARY(LIBGCRYPT_LIBRARIES_DEBUG 
			NAMES
			gcryptd
			PATHS
			${KDE4_LIB_DIR}
			${LIB_INSTALL_DIR}
		)
		
		if(NOT LIBGCRYPT_LIBRARIES_DEBUG)
			set(LIBGCRYPT_FOUND FALSE)
		endif(NOT LIBGCRYPT_LIBRARIES_DEBUG)
				
		if(MSVC_IDE)
			if( NOT LIBGCRYPT_LIBRARIES_DEBUG OR NOT LIBGCRYPT_LIBRARIES)
				message(FATAL_ERROR "\nCould NOT find the debug AND release version of the libgcrypt library.\nYou need to have both to use MSVC projects.\nPlease build and install both libgcrypt libraries first.\n")
			endif( NOT LIBGCRYPT_LIBRARIES_DEBUG OR NOT LIBGCRYPT_LIBRARIES)
		else(MSVC_IDE)
			string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
			if(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
				set(LIBGCRYPT_LIBRARIES ${LIBGCRYPT_LIBRARIES_DEBUG})
			else(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
				set(LIBGCRYPT_LIBRARIES ${LIBGCRYPT_LIBRARIES})
			endif(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
		endif(MSVC_IDE)
	endif(MSVC)
								
	if(LIBGCRYPT_FOUND)
		if(NOT LIBGCRYPT_FIND_QUIETLY)
			message(STATUS "Found libgcrypt: ${LIBGCRYPT_LIBRARIES} ")
		endif(NOT LIBGCRYPT_FIND_QUIETLY)
	else(LIBGCRYPT_FOUND)
		if(LIBGCRYPT_FIND_REQUIRED)
			if(NOT LIBGCRYPT_INCLUDE_DIR)
				message(FATAL_ERROR "Could not find libgcrypt includes.")
			endif(NOT LIBGCRYPT_INCLUDE_DIR)
			if(NOT LIBGCRYPT_LIBRARIES)
				message(FATAL_ERROR "Could not find libgcrypt library.")
			endif(NOT LIBGCRYPT_LIBRARIES)
		else(LIBGCRYPT_FIND_REQUIRED)
			if(NOT LIBGCRYPT_INCLUDE_DIR)
				message(STATUS "Could not find libgcrypt includes.")
			endif(NOT LIBGCRYPT_INCLUDE_DIR)
			if(NOT LIBGCRYPT_LIBRARIES)
				message(STATUS "Could not find libgcrypt library.")
			endif(NOT LIBGCRYPT_LIBRARIES)
		endif(LIBGCRYPT_FIND_REQUIRED)
	endif(LIBGCRYPT_FOUND)																	
endif(LIBGCRYPT_INCLUDE_DIR AND LIBGCRYPT_LIBRARIES)