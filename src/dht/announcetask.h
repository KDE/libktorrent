/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTANNOUNCETASK_H
#define DHTANNOUNCETASK_H

#include "kbucket.h"
#include "task.h"

namespace dht
{
class Database;

class KBucketEntryAndToken : public KBucketEntry
{
    QByteArray token;

public:
    KBucketEntryAndToken()
    {
    }
    KBucketEntryAndToken(const KBucketEntry &e, const QByteArray &token)
        : KBucketEntry(e)
        , token(token)
    {
    }
    ~KBucketEntryAndToken() override
    {
    }

    const QByteArray &getToken() const
    {
        return token;
    }
};

/*!
    \author Joris Guisson <joris.guisson@gmail.com>
*/
class AnnounceTask : public Task
{
public:
    AnnounceTask(Database *db, RPCServer *rpc, Node *node, const dht::Key &info_hash, bt::Uint16 port, QObject *parent);
    ~AnnounceTask() override;

    void callFinished(RPCCall *c, RPCMsg *rsp) override;
    void callTimeout(RPCCall *c) override;
    void update() override;

    /*!
     * Take one item from the returned values.
     * Returns false if there is no item to take.
     * \param item The item
     * \return false if no item to take, true else
     */
    bool takeItem(DBItem &item);

private:
    void handleNodes(const QByteArray &nodes, int ip_version);

private:
    dht::Key info_hash;
    bt::Uint16 port;
    std::set<KBucketEntryAndToken> answered; // nodes which have answered with values
    KBucketEntrySet answered_visited; // nodes which have answered with values which have been visited
    Database *db;
    DBItemList returned_items;
};

}

#endif
