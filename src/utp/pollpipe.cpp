/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pollpipe.h"
#include "connection.h"
#include <util/log.h>

using namespace bt;

namespace utp
{
PollPipe::PollPipe(net::Poll::Mode mode)
    : mode(mode)
    , poll_index(-1)
{
}

PollPipe::~PollPipe()
{
}

void PollPipe::reset()
{
    QMutexLocker lock(&mutex);
    poll_index = -1;
    conn_ids.reset();
}

}
