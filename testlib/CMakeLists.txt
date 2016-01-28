set(testlib_SRC
    utils.cpp
    dummytorrentcreator.cpp
)

add_library(testlib STATIC ${testlib_SRC})

target_include_directories(testlib PUBLIC "$<BUILD_INTERFACE:${LIBKTORRENT_SOURCE_DIR}/testlib;${LIBKTORRENT_SOURCE_DIR}>")

target_link_libraries(testlib ktorrent KF5::KIOCore KF5::Crash)
