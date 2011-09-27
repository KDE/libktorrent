set(circularbuffertest_SRCS circularbuffertest.cpp)
kde4_add_unit_test(circularbuffertest TESTNAME circularbuffertest ${circularbuffertest_SRCS})
target_link_libraries( circularbuffertest ${QT_QTTEST_LIBRARY} ktorrent)

if(NOT MSVC)
set(signalcatchertest_SRCS signalcatchertest.cpp)
kde4_add_unit_test(signalcatchertest TESTNAME signalcatchertest ${signalcatchertest_SRCS})
target_link_libraries( signalcatchertest ${QT_QTTEST_LIBRARY} ktorrent)
endif(NOT MSVC)

set(resourcemanagertest_SRCS resourcemanagertest.cpp)
kde4_add_unit_test(resourcemanagertest TESTNAME resourcemanagertest ${resourcemanagertest_SRCS})
target_link_libraries( resourcemanagertest ${QT_QTTEST_LIBRARY} ktorrent)

set(fileopstest_SRCS fileopstest.cpp)
kde4_add_unit_test(fileopstest TESTNAME fileopstest ${fileopstest_SRCS})
target_link_libraries( fileopstest ${KDE4_SOLID_LIBS} ${QT_QTTEST_LIBRARY} ktorrent)