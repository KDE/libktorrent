/***************************************************************************
 *   Copyright (C) 2012 by                                                 *
 *   Joris Guisson <joris.guisson@gmail.com>                               *
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
