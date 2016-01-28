set(testlib_SRC
    utils.cpp
    dummytorrentcreator.cpp
)

add_library(testlib STATIC ${testlib_SRC})
target_link_libraries(testlib KF5::KIOCore KF5::Crash)
