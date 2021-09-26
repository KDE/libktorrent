/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "utpserverthread.h"
#include "utpserver.h"
#include <QTimer>

namespace utp
{
UTPServerThread::UTPServerThread(UTPServer *srv)
    : srv(srv)
{
    srv->moveToThread(this);
}

UTPServerThread::~UTPServerThread()
{
}

void UTPServerThread::run()
{
    srv->threadStarted();
    exec();
}

}
