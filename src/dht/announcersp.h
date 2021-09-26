/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_ANNOUNCERSP_H
#define DHT_ANNOUNCERSP_H

#include "rpcmsg.h"

namespace dht
{
/**
 * Announce response message
 */
class KTORRENT_EXPORT AnnounceRsp : public RPCMsg
{
public:
    AnnounceRsp();
    AnnounceRsp(const QByteArray &mtid, const Key &id);
    ~AnnounceRsp() override;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;
    void parse(bt::BDictNode *dict) override;

    typedef QSharedPointer<AnnounceRsp> Ptr;
};
}

#endif // DHT_ANNOUNCERSP_H
