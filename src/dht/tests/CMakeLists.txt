set(rpcmsgtest_SRCS rpcmsgtest.cpp)

kde4_add_unit_test(rpcmsgtest TESTNAME rpcmsgtest ${rpcmsgtest_SRCS})
target_link_libraries( rpcmsgtest ${QT_QTTEST_LIBRARY} testlib ktorrent)