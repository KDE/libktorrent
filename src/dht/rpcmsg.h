/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTRPCMSG_H
#define DHTRPCMSG_H

#include "database.h"
#include "key.h"
#include <QString>
#include <ktorrent_export.h>
#include <util/constants.h>

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
    INVALID,
};

enum Method {
    PING,
    FIND_NODE,
    GET_PEERS,
    ANNOUNCE_PEER,
    NONE,
};

const QByteArray TID = QByteArrayLiteral("t");
const QByteArray REQ = "q";
const QByteArray RSP = "r";
const QByteArray TYP = "y";
const QByteArray ARG = "a";
const QByteArray ERR_DHT = "e";

/*!
 * \headerfile dht/rpcmsg.h
 * \brief Base class for all RPC messages.
 */
class KTORRENT_EXPORT RPCMsg
{
public:
    RPCMsg();
    RPCMsg(const QByteArray &mtid, Method m, Type type, const Key &id);
    virtual ~RPCMsg();

    /*!
     * When this message arrives this function will be called upon the DHT.
     * The message should then call the appropriate DHT function (double dispatch)
     * \param dh_table Pointer to DHT
     */
    virtual void apply(DHT *dh_table) = 0;

    /*!
     * Print the message for debugging purposes.
     */
    virtual void print() = 0;

    /*!
     * BEncode the message.
     * \param arr Data array
     */
    virtual void encode(QByteArray &arr) const = 0;

    /*!
     * Parse the message
     * \param dict Data dictionary
     * \throws bt::Error when something goes wrong
     **/
    virtual void parse(bt::BDictNode *dict);

    //! Set the origin (i.e. where the message came from)
    void setOrigin(const net::Address &o)
    {
        origin = o;
    }

    //! Get the origin
    [[nodiscard]] const net::Address &getOrigin() const
    {
        return origin;
    }

    //! Set the origin (i.e. where the message came from)
    void setDestination(const net::Address &o)
    {
        origin = o;
    }

    //! Get the origin
    [[nodiscard]] const net::Address &getDestination() const
    {
        return origin;
    }

    //! Get the MTID
    [[nodiscard]] const QByteArray &getMTID() const
    {
        return mtid;
    }

    //! Set the MTID
    void setMTID(const QByteArray &m)
    {
        mtid = m;
    }

    //! Get the id of the sender
    [[nodiscard]] const Key &getID() const
    {
        return id;
    }

    //! Get the type of the message
    [[nodiscard]] Type getType() const
    {
        return type;
    }

    //! Get the message it's method
    [[nodiscard]] Method getMethod() const
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
