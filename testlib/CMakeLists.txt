add_library(testlib STATIC)

target_sources(testlib PRIVATE
    utils.cpp
    dummytorrentcreator.cpp
)

target_include_directories(testlib PUBLIC "$<BUILD_INTERFACE:${LIBKTORRENT_SOURCE_DIR}/testlib;${LIBKTORRENT_SOURCE_DIR}>")

target_link_libraries(testlib KTorrent6 KF6::KIOCore KF6::Crash)
