include(ECMAddTests)
ecm_add_test(datacheckertest.cpp LINK_LIBRARIES testlib KF6::Torrent Qt6::Test)
