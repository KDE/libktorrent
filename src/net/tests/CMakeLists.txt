set(polltest_SRCS polltest.cpp)
kde4_add_unit_test(polltest TESTNAME polltest ${polltest_SRCS})
target_link_libraries( polltest ${QT_QTTEST_LIBRARY} ktorrent)

set(wakeuppipetest_SRCS wakeuppipetest.cpp)
kde4_add_unit_test(wakeuppipetest TESTNAME wakeuppipetest ${wakeuppipetest_SRCS})
target_link_libraries( wakeuppipetest ${QT_QTTEST_LIBRARY} ktorrent)