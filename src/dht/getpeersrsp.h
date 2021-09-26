/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_GETPEERSRSP_H
#define DHT_GETPEERSRSP_H

#include "packednodecontainer.h"
#include "rpcmsg.h"

namespace dht
{
/**
 * GetPeers response message
 */
class KTORRENT_EXPORT GetPeersRsp : public RPCMsg, public PackedNodeContainer
{
public:
    GetPeersRsp();
    GetPeersRsp(const QByteArray &mtid, const Key &id, const QByteArray &token);
    GetPeersRsp(const QByteArray &mtid, const Key &id, const DBItemList &values, const QByteArray &token);
    ~GetPeersRsp() override;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;
    void parse(bt::BDictNode *dict) override;

    const DBItemList &getItemList() const
    {
        return items;
    }
    const QByteArray &getToken() const
    {
        return token;
    }
    bool containsNodes() const
    {
        return nodes.size() > 0 || nodes6.size() > 0;
    }
    bool containsValues() const
    {
        return nodes.size() == 0;
    }

    typedef QSharedPointer<GetPeersRsp> Ptr;

private:
    QByteArray token;
    DBItemList items;
};

}

#endif // DHT_GETPEERSRSP_H
