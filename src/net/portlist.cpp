/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "portlist.h"
#include <QtAlgorithms>

namespace net
{
Port::Port()
    : number(0)
    , proto(TCP)
    , forward(false)
{
}

Port::Port(bt::Uint16 number, Protocol proto, bool forward)
    : number(number)
    , proto(proto)
    , forward(forward)
{
}

Port::Port(const Port &p)
    : number(p.number)
    , proto(p.proto)
    , forward(p.forward)
{
}

bool Port::operator==(const Port &p) const
{
    return number == p.number && proto == p.proto;
}

PortList::PortList()
    : lst(nullptr)
{
}

PortList::~PortList()
{
}

void PortList::addNewPort(bt::Uint16 number, Protocol proto, bool forward)
{
    Port p = Port(number, proto, forward);
    append(p);
    if (lst) {
        lst->portAdded(p);
    }
}

void PortList::removePort(bt::Uint16 number, Protocol proto)
{
    PortList::iterator itr = std::find(begin(), end(), Port(number, proto, false));
    if (itr == end()) {
        return;
    }

    if (lst) {
        lst->portRemoved(*itr);
    }

    erase(itr);
}

}
