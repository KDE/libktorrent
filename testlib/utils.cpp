/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "utils.h"

#include <QRandomGenerator64>
#include <QString>

#include <net/address.h>
#include <net/poll.h>
#include <net/socket.h>

using namespace Qt::StringLiterals;

bt::Uint64 RandomSize(bt::Uint64 min_size, bt::Uint64 max_size)
{
    bt::Uint64 r = max_size - min_size;
    return min_size + QRandomGenerator64::global()->generate() % r;
}

std::optional<SocketPair> CreateSocketPair(int ip_version)
{
    auto sock = std::make_unique<net::Socket>(true, ip_version);
    if (!sock->bind(ip_version == 4 ? u"127.0.0.1"_s : u"::1"_s, 0, true)) {
        return {};
    }

    net::Address local_addr = sock->getSockName();
    auto writer = std::make_unique<net::Socket>(true, ip_version);
    writer->setBlocking(false);
    writer->connectTo(local_addr);

    net::Address dummy;
    int fd = sock->accept(dummy);
    if (fd < 0) {
        return {};
    }

    if (!writer->connectSuccesFull()) {
        return {};
    }

    auto reader = std::make_unique<net::Socket>(fd, ip_version);

    return SocketPair{.reader = std::move(reader), .writer = std::move(writer)};
}
