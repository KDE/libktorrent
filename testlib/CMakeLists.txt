include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(testlib_SRC 
	utils.cpp 
	dummytorrentcreator.cpp
)

add_library(testlib STATIC ${testlib_SRC})
target_link_libraries(testlib KF5::KDELibs4Support)
