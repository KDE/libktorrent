/*
    SPDX-FileCopyrightText: 2009 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "net/wakeuppipe.h"
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace net
{
WakeUpPipe::WakeUpPipe()
    : woken_up(false)
{
}

WakeUpPipe::~WakeUpPipe()
{
}

void WakeUpPipe::wakeUp()
{
    QMutexLocker lock(&mutex);
    if (woken_up)
        return;

    char data[] = "d";
    if (bt::Pipe::write((const bt::Uint8 *)data, 1) != 1)
        Out(SYS_GEN | LOG_DEBUG) << "WakeUpPipe: wake up failed " << endl;
    else
        woken_up = true;
}

void WakeUpPipe::handleData()
{
    QMutexLocker lock(&mutex);
    bt::Uint8 buf[20];
    int ret = bt::Pipe::read(buf, 20);
    if (ret < 0)
        Out(SYS_GEN | LOG_DEBUG) << "WakeUpPipe: read failed " << endl;

    woken_up = false;
}

void WakeUpPipe::reset()
{
    // Do nothing
}

}
