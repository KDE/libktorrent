/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "dhtbase.h"

namespace dht
{
DHTBase::DHTBase()
    : running(false)
    , port(0)
{
    stats.num_peers = 0;
    stats.num_tasks = 0;
}

DHTBase::~DHTBase()
{
}
}
