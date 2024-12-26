/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "socketgroup.h"
#include "trafficshapedsocket.h"
#include <cmath>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace net
{
SocketGroup::SocketGroup(Uint32 limit, Uint32 assured_rate)
    : limit(limit)
    , assured_rate(assured_rate)
    , prev_run_time(bt::CurrentTime())
    , group_allowance(0)
    , group_assured(0)
{
}

SocketGroup::~SocketGroup()
{
}

void SocketGroup::processUnlimited(bool up, bt::TimeStamp now)
{
    std::list<TrafficShapedSocket *>::iterator i = sockets.begin();
    while (i != sockets.end()) {
        TrafficShapedSocket *s = *i;
        if (s) {
            if (up)
                s->write(0, now);
            else
                s->read(0, now);
        }
        ++i;
    }
}

bool SocketGroup::processLimited(bool up, bt::TimeStamp now, Uint32 &allowance)
{
    Uint32 bslot = allowance / sockets.size() + 1;

    std::list<TrafficShapedSocket *>::iterator itr = sockets.begin();

    // while we can send and there are sockets left to send
    while (sockets.size() > 0 && allowance > 0) {
        Uint32 as = bslot;
        if (as > allowance)
            as = allowance;

        TrafficShapedSocket *s = *itr;
        if (s) {
            Uint32 ret = 0;
            if (up)
                ret = s->write(as, now);
            else
                ret = s->read(as, now);

            // if this socket did what it was supposed to do,
            // it can have another go if stuff is leftover
            // if it doesn't, we erase it from the list
            if (ret != as)
                itr = sockets.erase(itr);
            else
                ++itr;

            if (ret > allowance)
                allowance = 0;
            else
                allowance -= ret;
        } else {
            // 0 pointer so just erase
            itr = sockets.erase(itr);
        }

        // wrap around if necessary
        if (itr == sockets.end())
            itr = sockets.begin();
    }

    return sockets.size() > 0;
}

bool SocketGroup::download(Uint32 &global_allowance, bt::TimeStamp now)
{
    return process(false, now, global_allowance);
}

bool SocketGroup::upload(Uint32 &global_allowance, bt::TimeStamp now)
{
    return process(true, now, global_allowance);
}

void SocketGroup::calcAllowance(bt::TimeStamp now)
{
    if (limit > 0)
        group_allowance = (Uint32)ceil(1.0 * limit * (now - prev_run_time) * 0.001);
    else
        group_allowance = 0;

    if (assured_rate > 0)
        group_assured = (Uint32)ceil(1.0 * assured_rate * (now - prev_run_time) * 0.001);
    else
        group_assured = 0;

    prev_run_time = now;
}

bool SocketGroup::process(bool up, bt::TimeStamp now, Uint32 &global_allowance)
{
    if (limit > 0) {
        if (group_allowance == 0) {
            clear();
            return false;
        }

        bool ret = false;
        if (global_allowance == 0) {
            Uint32 p = group_allowance;
            ret = processLimited(up, now, p);
            group_allowance = p;
        } else if (global_allowance <= group_allowance) {
            Uint32 tmp = global_allowance;
            ret = processLimited(up, now, tmp);

            Uint32 done = (global_allowance - tmp);
            if (group_allowance < done)
                group_allowance = 0;
            else
                group_allowance -= done;

            global_allowance = tmp;
        } else {
            Uint32 p = group_allowance;
            ret = processLimited(up, now, p);

            Uint32 done = (group_allowance - p);
            if (global_allowance < done)
                global_allowance = 0;
            else
                global_allowance -= done;

            group_allowance = p;
        }

        // if group allowance is used up, this group can no longer do anything
        if (group_allowance == 0) {
            clear();
            return false;
        } else
            return ret;
    } else if (global_allowance > 0) {
        return processLimited(up, now, global_allowance);
    } else {
        processUnlimited(up, now);
        return false;
    }
}

}
