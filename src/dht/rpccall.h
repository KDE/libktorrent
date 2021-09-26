/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTRPCCALL_H
#define DHTRPCCALL_H

#include "key.h"
#include "rpcmsg.h"
#include <qtimer.h>

namespace dht
{
class RPCCall;

/**
 * Class which objects should derive from, if they want to know the result of a call.
 */
class RPCCallListener : public QObject
{
public:
    RPCCallListener(QObject *parent);
    ~RPCCallListener() override;

    /**
     * A response was received.
     * @param c The call
     * @param rsp The response
     */
    virtual void onResponse(RPCCall *c, RPCMsg::Ptr rsp) = 0;

    /**
     * The call has timed out.
     * @param c The call
     */
    virtual void onTimeout(RPCCall *c) = 0;
};

/**
 * @author Joris Guisson
 */
class RPCCall : public QObject
{
    Q_OBJECT
public:
    RPCCall(RPCMsg::Ptr msg, bool queued);
    ~RPCCall() override;

    /**
     * Called when a queued call gets started. Starts the timeout timer.
     */
    void start();

    /**
     * Called by the server if a response is received.
     * @param rsp
     */
    void response(RPCMsg::Ptr rsp);

    /**
     * Add a listener for this call
     * @param cl The listener
     */
    void addListener(RPCCallListener *cl);

    /// Get the message type
    Method getMsgMethod() const;

    /// Get the request sent
    const RPCMsg::Ptr getRequest() const
    {
        return msg;
    }

    /// Get the request sent
    RPCMsg::Ptr getRequest()
    {
        return msg;
    }

private Q_SLOTS:
    void onTimeout();

Q_SIGNALS:
    void response(RPCCall *c, RPCMsg::Ptr rsp);
    void timeout(RPCCall *c);

private:
    RPCMsg::Ptr msg;
    QTimer timer;
    bool queued;
};

}

#endif
