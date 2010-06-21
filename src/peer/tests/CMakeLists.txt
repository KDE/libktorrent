set(superseedtest_SRCS superseedtest.cpp)
kde4_add_unit_test(superseedtest TESTNAME superseedtest ${superseedtest_SRCS})
target_link_libraries( superseedtest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktorrent)