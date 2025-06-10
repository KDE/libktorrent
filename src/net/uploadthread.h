/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETUPLOADTHREAD_H
#define NETUPLOADTHREAD_H

#include <QMutex>
#include <QWaitCondition>
#include <net/networkthread.h>
#include <net/wakeuppipe.h>

namespace net
{
class SocketMonitor;

/*!
    \author Joris Guisson <joris.guisson@gmail.com>
    \brief Thread which processes outgoing data.
*/
class UploadThread : public NetworkThread
{
    static bt::Uint32 ucap;
    static bt::Uint32 sleep_time;

    WakeUpPipe::Ptr wake_up;

public:
    UploadThread(SocketMonitor *sm);
    ~UploadThread() override;

    //! Wake up thread, data is ready to be sent
    void signalDataReady();

    //! Set the upload cap
    static void setCap(bt::Uint32 uc)
    {
        ucap = uc;
    }

    //! Get the upload cap
    static bt::Uint32 cap()
    {
        return ucap;
    }

    //! Set the sleep time when using upload caps
    static void setSleepTime(bt::Uint32 stime);

private:
    void update() override;
    bool doGroup(SocketGroup *g, bt::Uint32 &allowance, bt::TimeStamp now) override;

    int waitForSocketsReady();
};

}

#endif
