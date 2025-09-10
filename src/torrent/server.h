/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTSERVER_H
#define BTSERVER_H

#include "globals.h"
#include <QList>
#include <QObject>
#include <interfaces/serverinterface.h>
#include <ktorrent_export.h>

#include <memory>

namespace bt
{
class PeerManager;

/*!
 * \headerfile torrent/server.h
 * \author Joris Guisson
 *
 * \brief Listens for incoming connections, does the handshake, then hands off the new connections to a PeerManager.
 *
 * All PeerManager's should register with this class when they
 * are created and should unregister when they are destroyed.
 */
class KTORRENT_EXPORT Server : public ServerInterface
{
    Q_OBJECT
public:
    Server();
    ~Server() override;

    bool changePort(Uint16 port) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

}

#endif
