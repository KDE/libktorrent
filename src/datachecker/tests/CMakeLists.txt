set(datacheckertest_SRCS datacheckertest.cpp)

kde4_add_unit_test(datacheckertest TESTNAME datacheckertest ${datacheckertest_SRCS})
target_link_libraries( datacheckertest ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} testlib ktorrent)