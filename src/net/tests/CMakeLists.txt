include(ECMAddTests)
ecm_add_test(polltest.cpp LINK_LIBRARIES KF5::Torrent Qt5::Test)
ecm_add_test(wakeuppipetest.cpp LINK_LIBRARIES KF5::Torrent Qt5::Test)

