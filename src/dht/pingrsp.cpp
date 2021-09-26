/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pingrsp.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <util/log.h>

using namespace bt;

namespace dht
{
PingRsp::PingRsp()
    : RPCMsg(QByteArray(), PING, RSP_MSG, Key())
{
}

PingRsp::PingRsp(const QByteArray &mtid, const Key &id)
    : RPCMsg(mtid, PING, RSP_MSG, id)
{
}

PingRsp::~PingRsp()
{
}

void PingRsp::apply(dht::DHT *dh_table)
{
    dh_table->response(*this);
}

void PingRsp::print()
{
    Out(SYS_DHT | LOG_DEBUG) << QString("RSP: %1 %2 : ping").arg(mtid[0]).arg(id.toString()) << endl;
}

void PingRsp::encode(QByteArray &arr) const
{
    BEncoder enc(new BEncoderBufferOutput(arr));
    enc.beginDict();
    {
        enc.write(RSP);
        enc.beginDict();
        {
            enc.write(QByteArrayLiteral("id"));
            enc.write(id.getData(), 20);
        }
        enc.end();
        enc.write(TID);
        enc.write(mtid);
        enc.write(TYP);
        enc.write(RSP);
    }
    enc.end();
}
}
