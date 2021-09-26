/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETDOWNLOADTHREAD_H
#define NETDOWNLOADTHREAD_H

#include <net/networkthread.h>
#include <net/wakeuppipe.h>
#include <vector>

namespace net
{
/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Thread which processes incoming data
 */
class DownloadThread : public NetworkThread
{
public:
    DownloadThread(SocketMonitor *sm);
    ~DownloadThread() override;

    /// Wake up the download thread
    void wakeUp();

    /// Set the download cap
    static void setCap(bt::Uint32 cap)
    {
        dcap = cap;
    }

    /// Get the download cap
    static Uint32 cap()
    {
        return dcap;
    }

    /// Set the sleep time when using download caps
    static void setSleepTime(bt::Uint32 stime);

private:
    void update() override;
    bool doGroup(SocketGroup *g, Uint32 &allowance, bt::TimeStamp now) override;
    int waitForSocketReady();

private:
    WakeUpPipe::Ptr wake_up;

    static bt::Uint32 dcap;
    static bt::Uint32 sleep_time;
};

}

#endif
