/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_FINDNODERSP_H
#define DHT_FINDNODERSP_H

#include "packednodecontainer.h"
#include "rpcmsg.h"

namespace dht
{
/**
 * FindNode response message for DHT
 */
class KTORRENT_EXPORT FindNodeRsp : public RPCMsg, public PackedNodeContainer
{
public:
    FindNodeRsp();
    FindNodeRsp(const QByteArray &mtid, const Key &id);
    ~FindNodeRsp() override;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;
    void parse(bt::BDictNode *dict) override;

    typedef QSharedPointer<FindNodeRsp> Ptr;
};

}

#endif // DHT_FINDNODERSP_H
