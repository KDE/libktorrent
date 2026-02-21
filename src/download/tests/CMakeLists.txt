include(ECMAddTests)
ecm_add_test(packettest.cpp LINK_LIBRARIES testlib KTorrent6 Qt6::Test)
ecm_add_test(streamingchunkselectortest.cpp LINK_LIBRARIES testlib KTorrent6 Qt6::Test)
