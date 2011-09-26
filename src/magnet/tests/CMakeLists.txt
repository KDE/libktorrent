set(magnetlinktest_SRCS magnetlinktest.cpp)

kde4_add_unit_test(magnetlinktest TESTNAME magnetlinktest ${magnetlinktest_SRCS})
target_link_libraries( magnetlinktest ${QT_QTTEST_LIBRARY} ktorrent)