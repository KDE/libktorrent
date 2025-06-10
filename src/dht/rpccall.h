/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTRPCCALL_H
#define DHTRPCCALL_H

#include "key.h"
#include "rpcmsg.h"
#include <QTimer>

namespace dht
{
class RPCCall;

/*!
 * \brief Interface for classes that want to know the result of a call.
 */
class RPCCallListener : public QObject
{
public:
    RPCCallListener(QObject *parent);
    ~RPCCallListener() override;

    /*!
     * A response was received.
     * \param c The call
     * \param rsp The response
     */
    virtual void onResponse(RPCCall *c, RPCMsg *rsp) = 0;

    /*!
     * The call has timed out.
     * \param c The call
     */
    virtual void onTimeout(RPCCall *c) = 0;
};

/*!
 * \author Joris Guisson
 * \brief Notifies RPCCallListener when an RPCMsg times out or receives a response.
 */
class RPCCall : public QObject
{
    Q_OBJECT
public:
    RPCCall(std::unique_ptr<RPCMsg> msg, bool queued);
    ~RPCCall() override;

    /*!
     * Called when a queued call gets started. Starts the timeout timer.
     */
    void start();

    /*!
     * Called by the server if a response is received.
     * \param rsp
     */
    void response(RPCMsg *rsp);

    /*!
     * Add a listener for this call
     * \param cl The listener
     */
    void addListener(RPCCallListener *cl);

    //! Get the message type
    Method getMsgMethod() const;

    //! Get the request sent
    const RPCMsg *getRequest() const
    {
        return msg.get();
    }

    //! Get the request sent
    RPCMsg *getRequest()
    {
        return msg.get();
    }

private Q_SLOTS:
    void onTimeout();

Q_SIGNALS:
    void response(dht::RPCCall *c, dht::RPCMsg *rsp);
    void timeout(dht::RPCCall *c);

private:
    std::unique_ptr<RPCMsg> msg;
    QTimer timer;
    bool queued;
};

}

#endif
