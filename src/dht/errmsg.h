/*
    SPDX-FileCopyrightText: 2012 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_ERRMSG_H
#define DHT_ERRMSG_H

#include "rpcmsg.h"

namespace dht
{
/**
 * Error message in the DHT protocol
 */
class KTORRENT_EXPORT ErrMsg : public RPCMsg
{
public:
    ErrMsg();
    ErrMsg(const QByteArray &mtid, const dht::Key &id, const QString &msg);
    ~ErrMsg() override;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;
    void parse(bt::BDictNode *dict) override;

    /// Get the error message
    const QString &message() const
    {
        return msg;
    }

    typedef QSharedPointer<ErrMsg> Ptr;

private:
    QString msg;
};
}

#endif // DHT_ERRMSG_H
