include(ECMAddTests)
ecm_add_test(chunkmanagertest.cpp LINK_LIBRARIES testlib KTorrent6 Qt6::Test)
ecm_add_test(preallocationtest.cpp LINK_LIBRARIES testlib KTorrent6 Qt6::Test)
