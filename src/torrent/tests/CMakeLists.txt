include(ECMAddTests)

ecm_add_test(statsfiletest.cpp LINK_LIBRARIES KF5::Torrent Qt5::Test)
ecm_add_test(torrentfilestreamtest.cpp LINK_LIBRARIES testlib KF5::Torrent Qt5::Test)
ecm_add_test(torrentfilestreammultitest.cpp LINK_LIBRARIES testlib KF5::Torrent Qt5::Test)
