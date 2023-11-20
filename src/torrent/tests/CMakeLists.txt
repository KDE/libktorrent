include(ECMAddTests)

ecm_add_test(statsfiletest.cpp LINK_LIBRARIES KTorrent6 Qt6::Test)
ecm_add_test(torrentfilestreamtest.cpp LINK_LIBRARIES testlib KTorrent6 Qt6::Test)
ecm_add_test(torrentfilestreammultitest.cpp LINK_LIBRARIES testlib KTorrent6 Qt6::Test)
