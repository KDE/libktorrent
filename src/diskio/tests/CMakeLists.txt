include(ECMAddTests)
ecm_add_test(chunkmanagertest.cpp LINK_LIBRARIES testlib KF5::Torrent Qt5::Test)
ecm_add_test(preallocationtest.cpp LINK_LIBRARIES testlib KF5::Torrent Qt5::Test)