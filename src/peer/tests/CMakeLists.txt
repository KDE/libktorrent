include(ECMAddTests)
ecm_add_test(superseedtest.cpp LINK_LIBRARIES KF5::Torrent Qt5::Test)
ecm_add_test(packetreadertest.cpp LINK_LIBRARIES KF5::Torrent Qt5::Test)
ecm_add_test(connectionlimittest.cpp LINK_LIBRARIES KF5::Torrent Qt5::Test)
ecm_add_test(accessmanagertest.cpp LINK_LIBRARIES KF5::Torrent Qt5::Test)
