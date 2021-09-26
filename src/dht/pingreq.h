/*
    SPDX-FileCopyrightText: 2012 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_PINGREQ_H
#define DHT_PINGREQ_H

#include "rpcmsg.h"

namespace dht
{
/**
 * Ping request message in the DHT protocol
 */
class KTORRENT_EXPORT PingReq : public RPCMsg
{
public:
    PingReq();
    PingReq(const Key &id);
    ~PingReq() override;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;

    typedef QSharedPointer<PingReq> Ptr;
};

}

#endif // DHT_PINGREQ_H
