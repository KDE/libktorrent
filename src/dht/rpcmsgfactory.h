/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_RPCMSGFACTORY_H
#define DHT_RPCMSGFACTORY_H

#include "rpcmsg.h"
#include <ktorrent_export.h>

namespace dht
{
/*!
 * Interface to resolve the method of an RPC call given an mtid
 */
class RPCMethodResolver
{
public:
    virtual ~RPCMethodResolver()
    {
    }

    //! Return the method associated with an mtid
    virtual Method findMethod(const QByteArray &mtid) = 0;
};

/*!
 * Creates RPC message objects out of a BDictNode
 */
class KTORRENT_EXPORT RPCMsgFactory
{
public:
    RPCMsgFactory();
    virtual ~RPCMsgFactory();

    /*!
     * Creates a message out of a BDictNode.
     * \param dict The BDictNode
     * \param method_resolver The RPCMethodResolver
     * \return A newly created message
     * \throw bt::Error if something goes wrong
     */
    std::unique_ptr<RPCMsg> build(bt::BDictNode *dict, RPCMethodResolver *method_resolver);

private:
    std::unique_ptr<RPCMsg> buildRequest(bt::BDictNode *dict);
    std::unique_ptr<RPCMsg> buildResponse(bt::BDictNode *dict, RPCMethodResolver *method_resolver);
};

}

#endif // DHT_RPCMSGFACTORY_H
