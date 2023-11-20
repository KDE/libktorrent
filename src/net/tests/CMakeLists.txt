include(ECMAddTests)
ecm_add_test(polltest.cpp LINK_LIBRARIES KTorrent6 Qt6::Test)
ecm_add_test(wakeuppipetest.cpp LINK_LIBRARIES KTorrent6 Qt6::Test)

