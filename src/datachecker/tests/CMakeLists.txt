include(ECMAddTests)
ecm_add_test(datacheckertest.cpp LINK_LIBRARIES testlib KF5::Torrent Qt5::Test)
