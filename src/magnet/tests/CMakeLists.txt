set(magnetlinktest_SRCS magnetlinktest.cpp)

add_executable(magnetlinktestNAME magnetlinktest ${magnetlinktest_SRCS})
add_test(magnetlinktest magnetlinktest)
ecm_mark_as_test(magnetlinktest)
target_link_libraries( magnetlinktest Qt5::Test ktorrent)