include(ECMAddTests)
ecm_add_test(rpcmsgtest.cpp LINK_LIBRARIES testlib KF6::Torrent Qt6::Test)
ecm_add_test(keytest.cpp LINK_LIBRARIES testlib KF6::Torrent Qt6::Test)
