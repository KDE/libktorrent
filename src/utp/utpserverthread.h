/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTP_UTPSERVERTHREAD_H
#define UTP_UTPSERVERTHREAD_H

#include <QThread>
#include <ktorrent_export.h>

namespace utp
{
class UTPServer;

/*!
 * \brief Thread which runs a UTP server.
 */
class KTORRENT_EXPORT UTPServerThread : public QThread
{
    Q_OBJECT
public:
    UTPServerThread(UTPServer *srv);
    ~UTPServerThread() override;

    void run() override;

protected:
    UTPServer *srv;
};

}

#endif // UTP_UTPSERVERTHREAD_H
