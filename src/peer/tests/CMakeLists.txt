set(superseedtest_SRCS superseedtest.cpp)
kde4_add_unit_test(superseedtest TESTNAME superseedtest ${superseedtest_SRCS})
target_link_libraries( superseedtest ${QT_QTTEST_LIBRARY} ktorrent)