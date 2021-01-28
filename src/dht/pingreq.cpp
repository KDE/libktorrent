/***************************************************************************
 *   Copyright (C) 2012 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "pingreq.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <util/log.h>

using namespace bt;

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
    Out(SYS_DHT | LOG_DEBUG) << QString("REQ: %1 %2 : ping").arg(mtid[0]).arg(id.toString()) << endl;
}

void PingReq::encode(QByteArray &arr) const
{
    BEncoder enc(new BEncoderBufferOutput(arr));
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
