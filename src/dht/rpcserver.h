/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTRPCSERVER_H
#define DHTRPCSERVER_H

#include <QList>
#include <QObject>
#include <dht/rpcserverinterface.h>
#include <net/address.h>
#include <net/socket.h>
#include <util/constants.h>
#include <util/ptrmap.h>

using bt::Uint16;
using bt::Uint32;
using bt::Uint8;

namespace dht
{
class Key;
class DHT;

/**
 * @author Joris Guisson
 *
 * Class to handle incoming and outgoing RPC messages.
 */
class RPCServer : public QObject, public RPCServerInterface
{
    Q_OBJECT
public:
    RPCServer(DHT *dh_table, Uint16 port, QObject *parent = nullptr);
    ~RPCServer() override;

    /// Start the server
    void start();

    /// Stop the server
    void stop();

    /**
     * Do a RPC call.
     * @param msg The message to send
     * @return The call object
     */
    RPCCall *doCall(RPCMsg::Ptr msg) override;

    /**
     * Send a message, this only sends the message, it does not keep any call
     * information. This should be used for replies.
     * @param msg The message to send
     */
    void sendMsg(RPCMsg::Ptr msg);

    /**
     * Send a message, this only sends the message, it does not keep any call
     * information. This should be used for replies.
     * @param msg The message to send
     */
    void sendMsg(const RPCMsg &msg);

    /**
     * Ping a node, we don't care about the MTID.
     * @param addr The address
     */
    void ping(const dht::Key &our_id, const net::Address &addr);

    /// Get the number of active calls
    Uint32 getNumActiveRPCCalls() const;

private Q_SLOTS:
    void callTimeout(RPCCall *call);

private:
    class Private;
    Private *d;
};

}

#endif
