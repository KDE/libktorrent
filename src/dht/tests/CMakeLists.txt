include(ECMAddTests)
ecm_add_test(rpcmsgtest.cpp LINK_LIBRARIES testlib KF5::Torrent Qt5::Test)
ecm_add_test(keytest.cpp LINK_LIBRARIES testlib KF5::Torrent Qt5::Test)
