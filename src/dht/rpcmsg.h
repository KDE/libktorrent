/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#ifndef DHTRPCMSG_H
#define DHTRPCMSG_H

#include <QString>
#include <QSharedPointer>
#include <ktorrent_export.h>
#include <util/constants.h>
#include "key.h"
#include "database.h"

namespace bt
{
class BDictNode;
}

namespace dht
{

#define MAX_TOKEN_SIZE 40

class DHT;

enum Type {
    REQ_MSG,
    RSP_MSG,
    ERR_MSG,
    INVALID
};

enum Method {
    PING,
    FIND_NODE,
    GET_PEERS,
    ANNOUNCE_PEER,
    NONE
};

const QByteArray TID = QByteArrayLiteral("t");
const QByteArray REQ = "q";
const QByteArray RSP = "r";
const QByteArray TYP = "y";
const QByteArray ARG = "a";
const QByteArray ERR_DHT = "e";

/**
 * Base class for all RPC messages.
*/
class KTORRENT_EXPORT RPCMsg
{
public:
    RPCMsg();
    RPCMsg(const QByteArray & mtid, Method m, Type type, const Key & id);
    virtual ~RPCMsg();

    typedef QSharedPointer<RPCMsg> Ptr;

    /**
     * When this message arrives this function will be called upon the DHT.
     * The message should then call the appropriate DHT function (double dispatch)
     * @param dh_table Pointer to DHT
     */
    virtual void apply(DHT* dh_table) = 0;

    /**
     * Print the message for debugging purposes.
     */
    virtual void print() = 0;

    /**
     * BEncode the message.
     * @param arr Data array
     */
    virtual void encode(QByteArray & arr) const = 0;

    /**
     * Parse the message
     * @param dict Data dictionary
     * @throws bt::Error when something goes wrong
     **/
    virtual void parse(bt::BDictNode* dict);

    /// Set the origin (i.e. where the message came from)
    void setOrigin(const net::Address & o)
    {
        origin = o;
    }

    /// Get the origin
    const net::Address & getOrigin() const
    {
        return origin;
    }

    /// Set the origin (i.e. where the message came from)
    void setDestination(const net::Address & o)
    {
        origin = o;
    }

    /// Get the origin
    const net::Address & getDestination() const
    {
        return origin;
    }

    /// Get the MTID
    const QByteArray & getMTID() const
    {
        return mtid;
    }

    /// Set the MTID
    void setMTID(const QByteArray & m)
    {
        mtid = m;
    }

    /// Get the id of the sender
    const Key & getID() const
    {
        return id;
    }

    /// Get the type of the message
    Type getType() const
    {
        return type;
    }

    /// Get the message it's method
    Method getMethod() const
    {
        return method;
    }

protected:
    QByteArray mtid;
    Method method;
    Type type;
    Key id;
    net::Address origin;
};


}

#endif
