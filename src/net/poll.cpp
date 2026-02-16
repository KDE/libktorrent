/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "poll.h"
#include <util/log.h>

#ifndef Q_OS_WIN
#include <sys/poll.h>
#else
#include <Winsock2.h>
#endif

using namespace bt;

namespace net
{
Poll::Poll()
    : num_sockets(0)
{
}

Poll::~Poll()
{
}

int Poll::add(int fd, Poll::Mode mode)
{
    if (fd_vec.size() <= num_sockets) {
        // expand pollfd vector if necessary
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.revents = 0;
        pfd.events = mode == INPUT ? POLLIN : POLLOUT;
        fd_vec.push_back(pfd);
    } else {
        // use existing slot
        struct pollfd &pfd = fd_vec[num_sockets];
        pfd.fd = fd;
        pfd.revents = 0;
        pfd.events = mode == INPUT ? POLLIN : POLLOUT;
    }

    int ret = num_sockets;
    num_sockets++;
    return ret;
}

int Poll::add(PollClient::Ptr pc)
{
    int idx = add(pc->fd(), INPUT);
    poll_clients[idx] = pc;
    return idx;
}

bool Poll::ready(int index, Poll::Mode mode) const
{
    if (index < 0 || index >= (int)num_sockets) {
        return false;
    }

    return fd_vec[index].revents & (mode == INPUT ? POLLIN : POLLOUT);
}

void Poll::reset()
{
    num_sockets = 0;
}

int Poll::poll(int timeout)
{
    if (num_sockets == 0) {
        return 0;
    }

    int ret = 0;
#ifndef Q_OS_WIN
    ret = ::poll(&fd_vec[0], num_sockets, timeout);
#else
    ret = ::WSAPoll(&fd_vec[0], num_sockets, timeout);
#endif

    std::map<int, PollClient::Ptr>::iterator itr = poll_clients.begin();
    while (itr != poll_clients.end()) {
        if (ret > 0 && ready(itr->first, INPUT)) {
            itr->second->handleData();
        }
        itr->second->reset();
        ++itr;
    }

    poll_clients.clear();
    return ret;
}

}
