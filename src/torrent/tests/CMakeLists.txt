include_directories(${LIBKTORRENT_SOURCE_DIR})

set(statsfiletest_SRCS statsfiletest.cpp)
kde4_add_unit_test(statsfiletest TESTNAME statsfiletest ${statsfiletest_SRCS})
target_link_libraries( statsfiletest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktorrent)

set(torrentfilestreamtest_SRCS torrentfilestreamtest.cpp)
kde4_add_unit_test(torrentfilestreamtest TESTNAME torrentfilestreamtest ${torrentfilestreamtest_SRCS})
target_link_libraries( torrentfilestreamtest ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} testlib ktorrent)

set(torrentfilestreammultitest_SRCS torrentfilestreammultitest.cpp)
kde4_add_unit_test(torrentfilestreammultitest TESTNAME torrentfilestreammultitest ${torrentfilestreammultitest_SRCS})
target_link_libraries( torrentfilestreammultitest ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} testlib ktorrent)