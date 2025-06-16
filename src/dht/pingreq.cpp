/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pingreq.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
PingReq::PingReq()
    : RPCMsg(QByteArray(), PING, REQ_MSG, Key())
{
}

PingReq::PingReq(const Key &id)
    : RPCMsg(QByteArray(), PING, REQ_MSG, id)
{
}

PingReq::~PingReq()
{
}

void PingReq::apply(dht::DHT *dh_table)
{
    dh_table->ping(*this);
}

void PingReq::print()
{
    Out(SYS_DHT | LOG_DEBUG) << u"REQ: %1 %2 : ping"_s.arg(mtid[0]).arg(id.toString()) << endl;
}

void PingReq::encode(QByteArray &arr) const
{
    BEncoder enc(std::make_unique<BEncoderBufferOutput>(arr));
    enc.beginDict();
    {
        enc.write(ARG);
        enc.beginDict();
        {
            enc.write(QByteArrayLiteral("id"));
            enc.write(id.getData(), 20);
        }
        enc.end();
        enc.write(REQ);
        enc.write(QByteArrayLiteral("ping"));
        enc.write(TID);
        enc.write(mtid);
        enc.write(TYP);
        enc.write(REQ);
    }
    enc.end();
}
}
