include(ECMAddTests)
ecm_add_test(streamingchunkselectortest.cpp LINK_LIBRARIES testlib KF5::Torrent Qt::Test)
