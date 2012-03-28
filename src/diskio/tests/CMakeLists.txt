set(chunkmanagertest_SRCS chunkmanagertest.cpp)
kde4_add_unit_test(chunkmanagertest TESTNAME chunkmanagertest ${chunkmanagertest_SRCS})
target_link_libraries( chunkmanagertest ${QT_QTTEST_LIBRARY} testlib ktorrent)

set(preallocationtest_SRCS preallocationtest.cpp)
kde4_add_unit_test(preallocationtest TESTNAME preallocationtest ${preallocationtest_SRCS})
target_link_libraries( preallocationtest ${QT_QTTEST_LIBRARY} testlib ktorrent)