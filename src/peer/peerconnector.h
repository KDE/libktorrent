/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_PEERCONNECTOR_H
#define BT_PEERCONNECTOR_H

#include "connectionlimit.h"
#include <QSharedPointer>
#include <ktorrent_export.h>
#include <net/address.h>
#include <util/constants.h>
#include <util/resourcemanager.h>

#include <memory>

namespace bt
{
class Authenticate;
class PeerManager;

/*!
    Class which connects to a peer.
*/
class KTORRENT_EXPORT PeerConnector : public Resource
{
public:
    PeerConnector(const net::Address &addr, bool local, PeerManager *pman, ConnectionLimit::Token::Ptr token);
    ~PeerConnector() override;

    //! Called when an authentication attempt is finished
    void authenticationFinished(bt::Authenticate *auth, bool ok);

    //! Start connecting
    void start();

    /*!
     * Set the maximum number of active PeerConnectors allowed
     */
    static void setMaxActive(Uint32 mc);

    typedef QSharedPointer<PeerConnector> Ptr;
    typedef QWeakPointer<PeerConnector> WPtr;

    //! Set a weak pointer to this object
    void setWeakPointer(WPtr ptr);

private:
    void acquired() override;

    enum Method {
        TCP_WITH_ENCRYPTION,
        TCP_WITHOUT_ENCRYPTION,
        UTP_WITH_ENCRYPTION,
        UTP_WITHOUT_ENCRYPTION,
    };

    class Private;
    std::unique_ptr<Private> d;
};

}

#endif // BT_PEERCONNECTOR_H
