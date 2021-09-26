/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTSERVER_H
#define BTSERVER_H

#include "globals.h"
#include <interfaces/serverinterface.h>
#include <ktorrent_export.h>
#include <qlist.h>
#include <qobject.h>

namespace bt
{
class PeerManager;

/**
 * @author Joris Guisson
 *
 * Class which listens for incoming connections.
 * Handles authentication and then hands of the new
 * connections to a PeerManager.
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
    Private *d;
};

}

#endif
