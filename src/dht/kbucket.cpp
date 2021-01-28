/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include "kbucket.h"
#include "kclosestnodessearch.h"
#include "pingreq.h"
#include "rpcserverinterface.h"
#include "task.h"
#include <QHash>
#include <QtAlgorithms>
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <netinet/in.h>
#include <util/file.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace dht
{
KBucket::KBucket(RPCServerInterface *srv, const dht::Key &our_id)
    : RPCCallListener(0)
    , min_key(Key::min())
    , max_key(Key::max())
    , srv(srv)
    , our_id(our_id)
    , last_modified(bt::CurrentTime())
    , refresh_task(0)
{
}

KBucket::KBucket(const dht::Key &min_key, const dht::Key &max_key, dht::RPCServerInterface *srv, const dht::Key &our_id)
    : RPCCallListener(0)
    , min_key(min_key)
    , max_key(max_key)
    , srv(srv)
    , our_id(our_id)
    , last_modified(bt::CurrentTime())
    , refresh_task(0)
{
}

KBucket::~KBucket()
{
}

bool KBucket::keyInRange(const dht::Key &k) const
{
    return min_key <= k && k <= max_key;
}

bool KBucket::splitAllowed() const
{
    if (!keyInRange(our_id))
        return false;

    if (min_key + dht::K >= max_key)
        return false;

    return true;
}

std::pair<KBucket::Ptr, KBucket::Ptr> KBucket::split() noexcept(false)
{
    dht::Key m = dht::Key::mid(min_key, max_key);
    if (m == min_key || m + 1 == max_key)
        throw UnableToSplit();

    KBucket::Ptr left(new KBucket(min_key, m, srv, our_id));
    KBucket::Ptr right(new KBucket(m + 1, max_key, srv, our_id));

    QList<KBucketEntry>::iterator i;
    for (i = entries.begin(); i != entries.end(); ++i) {
        KBucketEntry &e = *i;
        if (left->keyInRange(e.getID()))
            left->insert(e);
        else
            right->insert(e);
    }

    return std::make_pair(left, right);
}

bool KBucket::insert(const KBucketEntry &entry)
{
    QList<KBucketEntry>::iterator i = std::find(entries.begin(), entries.end(), entry);

    // If in the list, move it to the end
    if (i != entries.end()) {
        KBucketEntry &e = *i;
        e.hasResponded();
        last_modified = bt::CurrentTime();
        entries.erase(i);
        entries.append(entry);
        return false;
    }

    // insert if not already in the list and we still have room
    if (i == entries.end() && entries.count() < (int)dht::K) {
        entries.append(entry);
        last_modified = bt::CurrentTime();
    } else if (!replaceBadEntry(entry)) {
        if (entries.count() == (int)dht::K && splitAllowed()) {
            // We can split
            return true;
        } else {
            // ping questionable nodes when replacing a bad one fails
            pingQuestionable(entry);
        }
    }

    return false;
}

void KBucket::onResponse(RPCCall *c, RPCMsg::Ptr rsp)
{
    Q_UNUSED(rsp);
    last_modified = bt::CurrentTime();

    if (!pending_entries_busy_pinging.contains(c))
        return;

    KBucketEntry entry = pending_entries_busy_pinging[c];
    pending_entries_busy_pinging.remove(c); // call is done so erase it

    // we have a response so try to find the next bad or questionable node
    // if we do not have room see if we can get rid of some bad peers
    if (!replaceBadEntry(entry)) // if no bad peers ping a questionable one
        pingQuestionable(entry);
}

void KBucket::onTimeout(RPCCall *c)
{
    if (!pending_entries_busy_pinging.contains(c))
        return;

    KBucketEntry entry = pending_entries_busy_pinging[c];

    // replace the entry which timed out
    QList<KBucketEntry>::iterator i;
    for (i = entries.begin(); i != entries.end(); ++i) {
        KBucketEntry &e = *i;
        if (e.getAddress() == c->getRequest()->getOrigin()) {
            last_modified = bt::CurrentTime();
            entries.erase(i);
            entries.append(entry);
            break;
        }
    }
    pending_entries_busy_pinging.remove(c); // call is done so erase it
    // see if we can do another pending entry
    if (pending_entries_busy_pinging.count() < 2 && pending_entries.count() > 0) {
        KBucketEntry pe = pending_entries.front();
        pending_entries.pop_front();
        if (!replaceBadEntry(pe)) // if no bad peers ping a questionable one
            pingQuestionable(pe);
    }
}

void KBucket::pingQuestionable(const KBucketEntry &replacement_entry)
{
    if (pending_entries_busy_pinging.count() >= 2) {
        pending_entries.append(replacement_entry); // lets not have to many pending_entries calls going on
        return;
    }

    QList<KBucketEntry>::iterator i;
    // we haven't found any bad ones so try the questionable ones
    for (i = entries.begin(); i != entries.end(); ++i) {
        KBucketEntry &e = *i;
        if (e.isQuestionable()) {
            Out(SYS_DHT | LOG_DEBUG) << "Pinging questionable node : " << e.getAddress().toString() << endl;
            RPCMsg::Ptr p(new PingReq(our_id));
            p->setDestination(e.getAddress());
            RPCCall *c = srv->doCall(p);
            if (c) {
                e.onPingQuestionable();
                c->addListener(this);
                // add the pending entry
                pending_entries_busy_pinging.insert(c, replacement_entry);
                return;
            }
        }
    }
}

bool KBucket::replaceBadEntry(const KBucketEntry &entry)
{
    QList<KBucketEntry>::iterator i;
    for (i = entries.begin(); i != entries.end(); ++i) {
        KBucketEntry &e = *i;
        if (e.isBad()) {
            // bad one get rid of it
            last_modified = bt::CurrentTime();
            entries.erase(i);
            entries.append(entry);
            return true;
        }
    }
    return false;
}

bool KBucket::contains(const KBucketEntry &entry) const
{
    return entries.contains(entry);
}

void KBucket::findKClosestNodes(KClosestNodesSearch &kns) const
{
    for (const KBucketEntry &i : qAsConst(entries)) {
        kns.tryInsert(i);
    }
}

bool KBucket::onTimeout(const net::Address &addr)
{
    QList<KBucketEntry>::iterator i;

    for (i = entries.begin(); i != entries.end(); ++i) {
        KBucketEntry &e = *i;
        if (e.getAddress() == addr) {
            e.requestTimeout();
            return true;
        }
    }
    return false;
}

bool KBucket::needsToBeRefreshed() const
{
    bt::TimeStamp now = bt::CurrentTime();
    if (last_modified > now) {
        last_modified = now;
        return false;
    }

    return !refresh_task && entries.count() > 0 && (now - last_modified > BUCKET_REFRESH_INTERVAL);
}

void KBucket::updateRefreshTimer()
{
    last_modified = bt::CurrentTime();
}

void KBucket::save(bt::BEncoder &enc)
{
    enc.beginDict();
    enc.write(QByteArrayLiteral("min"), min_key.toByteArray());
    enc.write(QByteArrayLiteral("max"), max_key.toByteArray());
    enc.write(QByteArrayLiteral("entries"));
    enc.beginList();
    QList<KBucketEntry>::iterator i;
    for (i = entries.begin(); i != entries.end(); ++i) {
        enc.beginDict();
        KBucketEntry &e = *i;

        enc.write(QByteArrayLiteral("id"), e.getID().toByteArray());
        enc.write(QByteArrayLiteral("address"));
        if (e.getAddress().ipVersion() == 4) {
            Uint8 tmp[6];
            bt::WriteUint32(tmp, 0, e.getAddress().toIPv4Address());
            bt::WriteUint16(tmp, 4, e.getAddress().port());
            enc.write(tmp, 6);
        } else {
            Uint8 tmp[18];
            memcpy(tmp, e.getAddress().toIPv6Address().c, 16);
            bt::WriteUint16(tmp, 16, e.getAddress().port());
            enc.write(tmp, 18);
        }

        enc.end();
    }

    enc.end();
    enc.end();
}

void KBucket::load(bt::BDictNode *dict)
{
    min_key = dht::Key(dict->getByteArray("min"));
    max_key = dht::Key(dict->getByteArray("max"));
    BListNode *entry_list = dict->getList("entries");
    if (!entry_list || entry_list->getNumChildren() == 0)
        return;

    for (Uint32 i = 0; i < entry_list->getNumChildren(); i++) {
        BDictNode *entry = entry_list->getDict(i);
        if (!entry)
            continue;

        Key id = Key(entry->getByteArray("id"));
        QByteArray addr = entry->getByteArray("address");
        if (addr.size() == 6) {
            entries.append(KBucketEntry(net::Address(bt::ReadUint32((const Uint8 *)addr.data(), 0), bt::ReadUint16((const Uint8 *)addr.data(), 4)), id));
        } else {
            Q_IPV6ADDR ip;
            memcpy(ip.c, addr.data(), 16);
            entries.append(KBucketEntry(net::Address(ip, bt::ReadUint16((const Uint8 *)addr.data(), 16)), id));
        }
    }
}

void KBucket::onFinished(Task *t)
{
    if (t == refresh_task)
        refresh_task = 0;
}

void KBucket::setRefreshTask(Task *t)
{
    refresh_task = t;
    if (refresh_task) {
        connect(refresh_task, &Task::finished, this, &KBucket::onFinished);
    }
}

}
