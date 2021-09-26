/*
    SPDX-FileCopyrightText: 2012 by
    Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_ANNOUNCEREQ_H
#define DHT_ANNOUNCEREQ_H

#include "getpeersreq.h"

namespace dht
{
/**
 * Announce request in the DHT protocol
 */
class KTORRENT_EXPORT AnnounceReq : public GetPeersReq
{
public:
    AnnounceReq();
    AnnounceReq(const Key &id, const Key &info_hash, bt::Uint16 port, const QByteArray &token);
    ~AnnounceReq() override;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;
    void parse(bt::BDictNode *dict) override;

    const QByteArray &getToken() const
    {
        return token;
    }
    bt::Uint16 getPort() const
    {
        return port;
    }

    typedef QSharedPointer<AnnounceReq> Ptr;

private:
    bt::Uint16 port;
    QByteArray token;
};

}

#endif // DHT_ANNOUNCEREQ_H
