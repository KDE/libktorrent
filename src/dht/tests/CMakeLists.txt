set(rpcmsgtest_SRCS rpcmsgtest.cpp)
kde4_add_unit_test(rpcmsgtest TESTNAME rpcmsgtest ${rpcmsgtest_SRCS})
target_link_libraries( rpcmsgtest ${QT_QTTEST_LIBRARY} testlib ktorrent)

set(keytest_SRCS keytest.cpp)
kde4_add_unit_test(keytest TESTNAME keytest ${keytest_SRCS})
target_link_libraries( keytest ${QT_QTTEST_LIBRARY} testlib ktorrent)