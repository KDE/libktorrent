/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "downloadthread.h"
#include "socketgroup.h"
#include "socketmonitor.h"
#include "trafficshapedsocket.h"
#include "wakeuppipe.h"
#include <QtGlobal>
#include <cmath>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace net
{
Uint32 DownloadThread::dcap = 0;
Uint32 DownloadThread::sleep_time = 50;

DownloadThread::DownloadThread(SocketMonitor *sm)
    : NetworkThread(sm)
    , wake_up(new WakeUpPipe())
{
}

DownloadThread::~DownloadThread()
{
}

void DownloadThread::update()
{
    if (waitForSocketReady() > 0) {
        bool group_limits = false;
        sm->lock();

        TimeStamp now = bt::Now();
        Uint32 num_ready = 0;
        SocketMonitor::Itr itr = sm->begin();
        while (itr != sm->end()) {
            TrafficShapedSocket *s = *itr;
            if (!s->socketDevice()) {
                ++itr;
                continue;
            }

            if (s->socketDevice()->ready(this, Poll::INPUT)) {
                // add to the correct group
                Uint32 gid = s->downloadGroupID();
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
            doGroups(num_ready, now, dcap);
        sm->unlock();

        // to prevent huge CPU usage sleep a bit if we are limited (either by a global limit or a group limit)
        if (dcap > 0 || group_limits) {
            TimeStamp diff = now - prev_run_time;
            if (diff < sleep_time)
                msleep(sleep_time - diff);
        }
        prev_run_time = now;
    }
}

void DownloadThread::setSleepTime(Uint32 stime)
{
    sleep_time = stime;
}

bool DownloadThread::doGroup(SocketGroup *g, Uint32 &allowance, bt::TimeStamp now)
{
    return g->download(allowance, now);
}

int DownloadThread::waitForSocketReady()
{
    sm->lock();

    reset();
    // Add the wake up pipe
    add(qSharedPointerCast<PollClient>(wake_up));

    // fill the poll vector with all sockets
    SocketMonitor::Itr itr = sm->begin();
    while (itr != sm->end()) {
        TrafficShapedSocket *s = *itr;
        if (s && s->socketDevice()) {
            s->socketDevice()->prepare(this, Poll::INPUT);
        }
        ++itr;
    }
    sm->unlock();
    return poll();
}

void DownloadThread::wakeUp()
{
    wake_up->wakeUp();
}
}
