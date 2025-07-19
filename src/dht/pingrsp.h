/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_PINGRSP_H
#define DHT_PINGRSP_H

#include "rpcmsg.h"

namespace dht
{
/*!
 * Ping response message in the DHT protocol
 */
class KTORRENT_EXPORT PingRsp : public RPCMsg
{
public:
    PingRsp();
    PingRsp(const QByteArray &mtid, const Key &id);
    ~PingRsp() override;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;
};
}

#endif // DHT_PINGRSP_H
