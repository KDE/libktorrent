/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
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
