/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "node.h"

#include "dht.h"
#include "kbuckettable.h"
#include "kclosestnodessearch.h"
#include "key.h"
#include "nodelookup.h"
#include "rpccall.h"
#include "rpcmsg.h"
#include "rpcserver.h"
#include <torrent/globals.h>
#include <util/error.h>
#include <util/file.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
class Node::Private
{
public:
    Private(RPCServer *srv)
        : srv(srv)
    {
        num_receives = 0;
        new_key = false;
    }

    ~Private()
    {
    }

    void saveKey(const dht::Key &key, const QString &key_file)
    {
        bt::File fptr;
        if (!fptr.open(key_file, u"wb"_s)) {
            Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Cannot open file " << key_file << " : " << fptr.errorString() << endl;
            return;
        }

        fptr.write(key.getData(), 20);
        fptr.close();
    }

    dht::Key loadKey(const QString &key_file)
    {
        bt::File fptr;
        if (!fptr.open(key_file, u"rb"_s)) {
            Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Cannot open file " << key_file << " : " << fptr.errorString() << endl;
            dht::Key r = dht::Key::random();
            saveKey(r, key_file);
            new_key = true;
            return r;
        }

        Uint8 data[20];
        if (fptr.read(data, 20) != 20) {
            dht::Key r = dht::Key::random();
            saveKey(r, key_file);
            new_key = true;
            return r;
        }

        new_key = false;
        return dht::Key(data);
    }

    QScopedPointer<KBucketTable> ipv4_table;
    QScopedPointer<KBucketTable> ipv6_table;
    RPCServer *srv;
    Uint32 num_receives;
    bool new_key;
};

Node::Node(RPCServer *srv, const QString &key_file)
    : d(std::make_unique<Private>(srv))
{
    num_entries = 0;
    our_id = d->loadKey(key_file);
    d->ipv4_table.reset(new KBucketTable(our_id));
    d->ipv6_table.reset(new KBucketTable(our_id));
}

Node::~Node()
{
}

void Node::received(dht::DHT *dh_table, const dht::RPCMsg &msg)
{
    if (msg.getOrigin().ipVersion() == 4)
        d->ipv4_table->insert(KBucketEntry(msg.getOrigin(), msg.getID()), d->srv);
    else
        d->ipv6_table->insert(KBucketEntry(msg.getOrigin(), msg.getID()), d->srv);

    d->num_receives++;
    if (d->num_receives == 3) {
        // do a node lookup upon our own id
        // when we insert the first entry in the table
        dh_table->findOwnNode();
    }

    num_entries = d->ipv4_table->numEntries() + d->ipv6_table->numEntries();
}

void Node::findKClosestNodes(KClosestNodesSearch &kns, bt::Uint32 want)
{
    if (want & WANT_IPV4)
        d->ipv4_table->findKClosestNodes(kns);
    if (want & WANT_IPV6)
        d->ipv6_table->findKClosestNodes(kns);
}

void Node::onTimeout(RPCMsg::Ptr msg)
{
    if (msg->getOrigin().ipVersion() == 4)
        d->ipv4_table->onTimeout(msg->getOrigin());
    else
        d->ipv6_table->onTimeout(msg->getOrigin());
}

void Node::refreshBuckets(DHT *dh_table)
{
    d->ipv4_table->refreshBuckets(dh_table);
    d->ipv6_table->refreshBuckets(dh_table);
}

void Node::saveTable(const QString &file)
{
    d->ipv4_table->saveTable(file + ".ipv4"_L1);
    d->ipv6_table->saveTable(file + ".ipv6"_L1);
}

void Node::loadTable(const QString &file)
{
    if (d->new_key) {
        d->new_key = false;
        bt::Delete(file + ".ipv4"_L1, true);
        bt::Delete(file + ".ipv6"_L1, true);
        Out(SYS_DHT | LOG_IMPORTANT) << "DHT: new key, so removing tables" << endl;
    } else {
        d->ipv4_table->loadTable(file + ".ipv4"_L1, d->srv);
        d->ipv6_table->loadTable(file + ".ipv6"_L1, d->srv);
        num_entries = d->ipv4_table->numEntries() + d->ipv6_table->numEntries();
    }
}
}

#include "moc_node.cpp"
