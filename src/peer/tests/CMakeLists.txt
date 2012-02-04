set(superseedtest_SRCS superseedtest.cpp)
kde4_add_unit_test(superseedtest TESTNAME superseedtest ${superseedtest_SRCS})
target_link_libraries( superseedtest ${QT_QTTEST_LIBRARY} ktorrent)

set(packetreadertest_SRCS packetreadertest.cpp)
kde4_add_unit_test(packetreadertest TESTNAME packetreadertest ${packetreadertest_SRCS})
target_link_libraries(packetreadertest ${QT_QTTEST_LIBRARY} ktorrent)