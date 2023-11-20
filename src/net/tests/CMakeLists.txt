include(ECMAddTests)
ecm_add_test(polltest.cpp LINK_LIBRARIES KF6::Torrent Qt6::Test)
ecm_add_test(wakeuppipetest.cpp LINK_LIBRARIES KF6::Torrent Qt6::Test)

