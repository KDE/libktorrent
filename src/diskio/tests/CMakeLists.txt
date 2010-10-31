set(chunkmanagertest_SRCS chunkmanagertest.cpp)

kde4_add_unit_test(chunkmanagertest TESTNAME chunkmanagertest ${chunkmanagertest_SRCS})
target_link_libraries( chunkmanagertest ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} testlib ktorrent)