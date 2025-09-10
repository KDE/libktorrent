/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_FINDNODEREQ_H
#define DHT_FINDNODEREQ_H

#include "rpcmsg.h"
#include <QStringList>

namespace dht
{
/*!
 * \headerfile dht/findnodereq.h
 * \brief FindNode request in the DHT protocol.
 */
class KTORRENT_EXPORT FindNodeReq : public RPCMsg
{
public:
    FindNodeReq();
    FindNodeReq(const Key &id, const Key &target);
    ~FindNodeReq() override;

    void apply(DHT *dh_table) override;
    void print() override;
    void encode(QByteArray &arr) const override;
    void parse(bt::BDictNode *dict) override;

    [[nodiscard]] const Key &getTarget() const
    {
        return target;
    }
    [[nodiscard]] bool wants(int ip_version) const;

private:
    Key target;
    QStringList want;
};
}

#endif // DHT_FINDNODEREQ_H
