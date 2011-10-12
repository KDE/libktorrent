set(localwindowtest_SRCS localwindowtest.cpp)
kde4_add_unit_test(localwindowtest TESTNAME localwindowtest ${localwindowtest_SRCS})
target_link_libraries( localwindowtest ${QT_QTTEST_LIBRARY} ktorrent)

set(remotewindowtest_SRCS remotewindowtest.cpp)
kde4_add_unit_test(remotewindowtest TESTNAME remotewindowtest ${remotewindowtest_SRCS})
target_link_libraries( remotewindowtest ${QT_QTTEST_LIBRARY} ktorrent)

set(connectiontest_SRCS connectiontest.cpp)
kde4_add_unit_test(connectiontest TESTNAME connectiontest ${connectiontest_SRCS})
target_link_libraries( connectiontest ${QT_QTTEST_LIBRARY} ktorrent)

set(sendtest_SRCS sendtest.cpp)
kde4_add_unit_test(sendtest TESTNAME sendtest ${sendtest_SRCS})
target_link_libraries( sendtest ${QT_QTTEST_LIBRARY} ktorrent)

set(connecttest_SRCS connecttest.cpp)
kde4_add_unit_test(connecttest TESTNAME connecttest ${connecttest_SRCS})
target_link_libraries( connecttest ${QT_QTTEST_LIBRARY} ktorrent)

set(transmittest_SRCS transmittest.cpp)
kde4_add_unit_test(transmittest TESTNAME transmittest ${transmittest_SRCS})
target_link_libraries( transmittest ${QT_QTTEST_LIBRARY} ktorrent)

set(packetlosstest_SRCS packetlosstest.cpp)
kde4_add_unit_test(packetlosstest TESTNAME packetlosstest ${packetlosstest_SRCS})
target_link_libraries( packetlosstest ${QT_QTTEST_LIBRARY} ktorrent)

set(fintest_SRCS fintest.cpp)
kde4_add_unit_test(fintest TESTNAME fintest ${fintest_SRCS})
target_link_libraries( fintest ${QT_QTTEST_LIBRARY} ktorrent)

set(sockettest_SRCS sockettest.cpp)
kde4_add_unit_test(sockettest TESTNAME sockettest ${sockettest_SRCS})
target_link_libraries( sockettest ${QT_QTTEST_LIBRARY} ktorrent)

set(utppolltest_SRCS utppolltest.cpp)
kde4_add_unit_test(utppolltest TESTNAME utppolltest ${utppolltest_SRCS})
target_link_libraries( utppolltest ${QT_QTTEST_LIBRARY} ktorrent)

set(delaywindowtest_SRCS delaywindowtest.cpp)
kde4_add_unit_test(delaywindowtest TESTNAME delaywindowtest ${delaywindowtest_SRCS})
target_link_libraries( delaywindowtest ${QT_QTTEST_LIBRARY} ktorrent)

set(packetbuffertest_SRCS packetbuffertest.cpp)
kde4_add_unit_test(packetbuffertest TESTNAME packetbuffertest ${packetbuffertest_SRCS})
target_link_libraries( packetbuffertest ${QT_QTTEST_LIBRARY} ktorrent)

set(timevaluetest_SRCS timevaluetest.cpp)
kde4_add_unit_test(timevaluetest TESTNAME timevaluetest ${timevaluetest_SRCS})
target_link_libraries(timevaluetest ${QT_QTTEST_LIBRARY} ktorrent)