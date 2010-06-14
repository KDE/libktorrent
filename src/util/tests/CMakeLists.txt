set(circularbuffertest_SRCS circularbuffertest.cpp)
kde4_add_unit_test(circularbuffertest TESTNAME circularbuffertest ${circularbuffertest_SRCS})
target_link_libraries( circularbuffertest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktorrent)

set(signalcatchertest_SRCS signalcatchertest.cpp)
kde4_add_unit_test(signalcatchertest TESTNAME signalcatchertest ${signalcatchertest_SRCS})
target_link_libraries( signalcatchertest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktorrent)