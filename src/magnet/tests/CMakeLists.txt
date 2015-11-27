set(magnetlinktest_SRCS magnetlinktest.cpp)

add_executable(magnetlinktest ${magnetlinktest_SRCS})
add_test(magnetlinktest magnetlinktest)
ecm_mark_as_test(magnetlinktest)
target_link_libraries( magnetlinktest Qt5::Test ktorrent)