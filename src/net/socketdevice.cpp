/*
    SPDX-FileCopyrightText: 2009 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "socketdevice.h"

namespace net
{
SocketDevice::SocketDevice(bt::TransportProtocol proto)
    : m_state(IDLE)
    , remote_addr_override(false)
    , transport_protocol(proto)
{
}

SocketDevice::~SocketDevice()
{
}

}
