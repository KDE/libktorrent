/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "uploadthread.h"
#include "socketgroup.h"
#include "socketmonitor.h"
#include "trafficshapedsocket.h"
#include <cmath>
#include <util/functions.h>

using namespace bt;

namespace net
{
Uint32 UploadThread::ucap = 0;
Uint32 UploadThread::sleep_time = 50;

UploadThread::UploadThread(SocketMonitor *sm)
    : NetworkThread(sm)
    , wake_up(new WakeUpPipe())
{
}

UploadThread::~UploadThread()
{
}

void UploadThread::update()
{
    if (waitForSocketsReady() <= 0)
        return;

    bool group_limits = false;
    sm->lock();

    TimeStamp now = bt::Now();
    Uint32 num_ready = 0;
    SocketMonitor::Itr itr = sm->begin();
    while (itr != sm->end()) {
        TrafficShapedSocket *s = *itr;
        if (!s->socketDevice() || !s->socketDevice()->ok()) {
            ++itr;
            continue;
        }

        if (s->socketDevice()->ready(this, Poll::OUTPUT)) {
            // add to the correct group
            Uint32 gid = s->uploadGroupID();
            if (gid > 0)
                group_limits = true;

            SocketGroup *g = groups.find(gid);
            if (!g)
                g = groups.find(0);

            g->add(s);
            num_ready++;
        }
        ++itr;
    }

    if (num_ready > 0)
        doGroups(num_ready, now, ucap);
    sm->unlock();

    // to prevent huge CPU usage sleep a bit if we are limited (either by a global limit or a group limit)
    if (ucap > 0 || group_limits) {
        TimeStamp diff = now - prev_run_time;
        if (diff < sleep_time)
            msleep(sleep_time - diff);
    }
    prev_run_time = now;
}

void UploadThread::signalDataReady()
{
    wake_up->wakeUp();
}

void UploadThread::setSleepTime(Uint32 stime)
{
    sleep_time = stime;
}

bool UploadThread::doGroup(SocketGroup *g, Uint32 &allowance, bt::TimeStamp now)
{
    return g->upload(allowance, now);
}

int UploadThread::waitForSocketsReady()
{
    sm->lock();
    reset();
    // Add the wake up pipe
    add(qSharedPointerCast<PollClient>(wake_up));

    // fill the poll vector with all sockets
    SocketMonitor::Itr itr = sm->begin();
    while (itr != sm->end()) {
        TrafficShapedSocket *s = *itr;
        if (s && s->socketDevice()->ok() && s->bytesReadyToWrite()) {
            s->socketDevice()->prepare(this, Poll::OUTPUT);
        }
        ++itr;
    }
    sm->unlock();
    return poll();
}

}
