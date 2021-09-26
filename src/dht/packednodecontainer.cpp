/*
    SPDX-FileCopyrightText: 2012 by
    Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "packednodecontainer.h"

namespace dht
{
PackedNodeContainer::PackedNodeContainer()
{
}
PackedNodeContainer::~PackedNodeContainer()
{
}

void PackedNodeContainer::addNode(const QByteArray &a)
{
    if (a.size() == 26)
        nodes.append(a);
    else
        nodes6.append(a);
}

}
