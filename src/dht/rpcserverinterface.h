/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_RPCSERVERINTERFACE_H
#define DHT_RPCSERVERINTERFACE_H

#include <dht/rpcmsg.h>

namespace dht
{
class RPCCall;

/**
 * Interface class for an RPCServer
 */
class RPCServerInterface
{
public:
    RPCServerInterface();
    virtual ~RPCServerInterface();

    /**
     * Do a RPC call.
     * @param msg The message to send
     * @return The call object
     */
    virtual RPCCall *doCall(RPCMsg::Ptr msg) = 0;
};

}

#endif // DHT_RPCSERVERINTERFACE_H
