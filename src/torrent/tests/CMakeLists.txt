include(ECMAddTests)

ecm_add_test(statsfiletest.cpp LINK_LIBRARIES KF6::Torrent Qt6::Test)
ecm_add_test(torrentfilestreamtest.cpp LINK_LIBRARIES testlib KF6::Torrent Qt6::Test)
ecm_add_test(torrentfilestreammultitest.cpp LINK_LIBRARIES testlib KF6::Torrent Qt6::Test)
