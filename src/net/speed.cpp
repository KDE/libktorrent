/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "speed.h"
#include <util/functions.h>
#include <util/log.h>
#include <util/timer.h>

using namespace bt;

namespace net
{
const Uint64 SPEED_INTERVAL = 5000;

Speed::Speed()
    : rate(0)
    , bytes(0)
{
}

Speed::~Speed()
{
}

void Speed::onData(Uint32 b, bt::TimeStamp ts)
{
    dlrate.push_back(qMakePair(b, ts));
    bytes += b;
}

void Speed::update(bt::TimeStamp now)
{
    auto i = dlrate.begin();
    while (i != dlrate.end()) {
        QPair<Uint32, TimeStamp> &p = *i;
        if (now - p.second > SPEED_INTERVAL || now < p.second) {
            if (bytes >= p.first) // make sure we don't wrap around
                bytes -= p.first; // subtract bytes
            else
                bytes = 0;
            dlrate.pop_front();
            i = dlrate.begin();
        } else {
            // seeing that newer entries are appended, they are in the list chronologically
            // so once we hit an entry which is in the interval, we can just break out of the loop
            // because all following entries will be in the interval
            break;
        }
    }

    if (bytes == 0) {
        rate = 0;
    } else {
        //  Out() << "bytes = " << bytes << " d = " << d << endl;
        rate = bytes / (SPEED_INTERVAL / 1000);
    }
}

}
