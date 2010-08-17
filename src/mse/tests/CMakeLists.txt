set(rc4encryptortest_SRCS rc4encryptortest.cpp)
kde4_add_unit_test(rc4encryptortest TESTNAME rc4encryptortest ${rc4encryptortest_SRCS})
target_link_libraries( rc4encryptortest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktorrent)