# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Jack Hill <jackhill3103@gmail.com>

include(ECMAddTests)
ecm_add_test(bdecodertest.cpp LINK_LIBRARIES KTorrent6 Qt6::Test)
ecm_add_test(bencodertest.cpp LINK_LIBRARIES KTorrent6 Qt6::Test)
