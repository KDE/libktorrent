/***************************************************************************
 *   Copyright (C) 2010 by Joris Guisson                                   *
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

#ifndef BT_PEERCONNECTOR_H
#define BT_PEERCONNECTOR_H

#include <QSharedPointer>
#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/resourcemanager.h>
#include <net/address.h>
#include "connectionlimit.h"

#if (QT_VERSION < QT_VERSION_CHECK(4, 7, 0))
template <class T>
uint qHash(const QSharedPointer<T> &ptr)
{
    return qHash<T>(ptr.data());
}
#endif


namespace bt
{
class Authenticate;
class PeerManager;

/**
    Class which connects to a peer.
*/
class KTORRENT_EXPORT PeerConnector : public Resource
{
public:
    enum Method {
        TCP_WITH_ENCRYPTION,
        TCP_WITHOUT_ENCRYPTION,
        UTP_WITH_ENCRYPTION,
        UTP_WITHOUT_ENCRYPTION,
    };

    PeerConnector(const net::Address & addr, bool local, PeerManager* pman, ConnectionLimit::Token::Ptr token);
    ~PeerConnector() override;

    /// Called when an authentication attempt is finished
    void authenticationFinished(bt::Authenticate* auth, bool ok);

    /// Start connecting
    void start();

    /**
     * Set the maximum number of active PeerConnectors allowed
     */
    static void setMaxActive(Uint32 mc);

    typedef QSharedPointer<PeerConnector> Ptr;
    typedef QWeakPointer<PeerConnector> WPtr;

    /// Set a weak pointer to this object
    void setWeakPointer(WPtr ptr);


private:
    void acquired() override;

private:
    class Private;
    Private* d;
};

}

#endif // BT_PEERCONNECTOR_H
