/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "database.h"
#include <arpa/inet.h>
#include <torrent/globals.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace dht
{
DBItem::DBItem()
{
    time_stamp = bt::CurrentTime();
}

DBItem::DBItem(const net::Address &addr)
    : addr(addr)
{
    time_stamp = bt::CurrentTime();
}

DBItem::DBItem(const DBItem &it)
{
    addr = it.addr;
    time_stamp = it.time_stamp;
}

DBItem::~DBItem()
{
}

bool DBItem::expired(bt::TimeStamp now) const
{
    return (now - time_stamp >= MAX_ITEM_AGE);
}

DBItem &DBItem::operator=(const DBItem &it)
{
    addr = it.addr;
    time_stamp = it.time_stamp;
    return *this;
}

Uint32 DBItem::pack(Uint8 *buf) const
{
    if (addr.ipVersion() == 4) {
        WriteUint32(buf, 0, addr.toIPv4Address());
        WriteUint16(buf, 4, addr.port());
        return 6;
    } else {
        memcpy(buf, addr.toIPv6Address().c, 16);
        WriteUint16(buf, 16, addr.port());
        return 18;
    }
}

///////////////////////////////////////////////

Database::Database()
{
    items.setAutoDelete(true);
}

Database::~Database()
{
}

void Database::store(const dht::Key &key, const DBItem &dbi)
{
    DBItemList *dbl = items.find(key);
    if (!dbl) {
        dbl = new DBItemList();
        items.insert(key, dbl);
    }
    dbl->append(dbi);
}

void Database::sample(const dht::Key &key, DBItemList &tdbl, bt::Uint32 max_entries, bt::Uint32 ip_version)
{
    DBItemList *dbl = items.find(key);
    if (!dbl)
        return;

    DBItemList::iterator i = dbl->begin();
    while (i != dbl->end() && tdbl.count() < (int)max_entries) {
        if (ip_version == (bt::Uint32)i->getAddress().ipVersion())
            tdbl.append(*i);
        ++i;
    }
}

void Database::expire(bt::TimeStamp now)
{
    bt::PtrMap<dht::Key, DBItemList>::iterator itr = items.begin();
    while (itr != items.end()) {
        DBItemList *dbl = itr->second;
        // newer keys are inserted at the back
        // so we can stop when we hit the first key which is not expired
        while (dbl->count() > 0 && dbl->first().expired(now)) {
            dbl->pop_front();
        }
        if (dbl->count() == 0) {
            // PtrMap::erase(PtrMap::iterator) does not auto-delete
            delete dbl;
            itr = items.erase(itr);
        } else {
            ++itr;
        }
    }

    QMap<QByteArray, bt::TimeStamp>::iterator token_itr = tokens.begin();
    while (token_itr != tokens.end()) {
        if (now - token_itr.value() >= MAX_ITEM_AGE) {
            token_itr = tokens.erase(token_itr);
        } else {
            ++token_itr;
        }
    }
}

QByteArray Database::genToken(const net::Address &addr)
{
    if (addr.ipVersion() == 4) {
        Uint8 tdata[14];
        TimeStamp now = bt::CurrentTime();
        // generate a hash of the ip port and the current time
        // should prevent anybody from crapping things up
        bt::WriteUint32(tdata, 0, addr.toIPv4Address());
        bt::WriteUint16(tdata, 4, addr.port());
        bt::WriteUint64(tdata, 6, now);

        QByteArray token = SHA1Hash::generate(tdata, 14).toByteArray();
        // keep track of the token, tokens will expire after a while
        tokens.insert(token, now);
        return token;
    } else {
        Uint8 tdata[26];
        TimeStamp now = bt::CurrentTime();
        // generate a hash of the ip port and the current time
        // should prevent anybody from crapping things up
        memcpy(tdata, addr.toIPv6Address().c, 16);
        bt::WriteUint16(tdata, 16, addr.port());
        bt::WriteUint64(tdata, 18, now);

        QByteArray token = SHA1Hash::generate(tdata, 26).toByteArray();
        // keep track of the token, tokens will expire after a while
        tokens.insert(token, now);
        return token;
    }
}

bool Database::checkToken(const QByteArray &token, const net::Address &addr)
{
    // the token must be in the map
    if (!tokens.contains(token))
        return false;

    // in the map so now get the timestamp and regenerate the token
    // using the IP and port of the sender
    TimeStamp ts = tokens[token];

    if (addr.ipVersion() == 4) {
        Uint8 tdata[14];
        bt::WriteUint32(tdata, 0, addr.toIPv4Address());
        bt::WriteUint16(tdata, 4, addr.port());
        bt::WriteUint64(tdata, 6, ts);
        QByteArray ct = SHA1Hash::generate(tdata, 14).toByteArray();

        // compare the generated token to the one received
        if (token != ct) { // not good, this peer didn't went through the proper channels
            Out(SYS_DHT | LOG_DEBUG) << "Invalid token" << endl;
            return false;
        }
    } else {
        Uint8 tdata[26];

        memcpy(tdata, addr.toIPv6Address().c, 16);
        bt::WriteUint16(tdata, 16, addr.port());
        bt::WriteUint64(tdata, 18, ts);

        QByteArray ct = SHA1Hash::generate(tdata, 26).toByteArray();
        // compare the generated token to the one received
        if (token != ct) { // not good, this peer didn't went through the proper channels
            Out(SYS_DHT | LOG_DEBUG) << "Invalid token" << endl;
            return false;
        }
    }

    // expire the token
    tokens.remove(token);
    return true;
}

bool Database::contains(const dht::Key &key) const
{
    return items.find(key) != nullptr;
}

void Database::insert(const dht::Key &key)
{
    DBItemList *dbl = items.find(key);
    if (!dbl) {
        dbl = new DBItemList();
        items.insert(key, dbl);
    }
}
}
