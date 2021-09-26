/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_GETPEERSREQ_H
#define DHT_GETPEERSREQ_H

#include "rpcmsg.h"
#include <QStringList>

namespace dht
{
/**
 * GetPeers request in the DHT protocol
 */
class KTORRENT_EXPORT GetPeersReq : public RPCMsg
{
public:
    GetPeersReq();
    GetPeersReq(const Key &id, const Key &info_hash);
    ~GetPeersReq() override;

    const Key &getInfoHash() const
    {
        return info_hash;
    }
    bool wants(int ip_version) const;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;
    void parse(bt::BDictNode *dict) override;

    typedef QSharedPointer<GetPeersReq> Ptr;

protected:
    Key info_hash;
    QStringList want;
};

}

#endif // DHT_GETPEERSREQ_H
