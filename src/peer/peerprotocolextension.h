/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_PEERPROTOCOLEXTENSION_H
#define BT_PEERPROTOCOLEXTENSION_H

#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
class Peer;

const Uint32 UT_PEX_ID = 1;
const Uint32 UT_METADATA_ID = 2;

/**
    Base class for protocol extensions
*/
class KTORRENT_EXPORT PeerProtocolExtension
{
public:
    PeerProtocolExtension(bt::Uint32 id, Peer *peer);
    virtual ~PeerProtocolExtension();

    /// Virtual update function does nothing, needs to be overridden if update
    virtual void update();

    /// Does this needs to be update
    virtual bool needsUpdate() const
    {
        return false;
    }

    /// Handle a packet
    virtual void handlePacket(QByteArrayView data) = 0;

    /// Send an extension protocol packet
    void sendPacket(const QByteArray &data);

    /// Change the ID
    void changeID(Uint32 id);

protected:
    bt::Uint32 id;
    Peer *peer;
};

}

#endif // BT_PEERPROTOCOLEXTENSION_H
