set(chunkmanagertest_SRCS chunkmanagertest.cpp)

kde4_add_unit_test(chunkmanagertest TESTNAME chunkmanagertest ${chunkmanagertest_SRCS})
target_link_libraries( chunkmanagertest ${QT_QTTEST_LIBRARY} testlib ktorrent)