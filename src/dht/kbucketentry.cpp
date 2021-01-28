/***************************************************************************
 *   Copyright (C) 2012 by                                                 *
 *   Joris Guisson <joris.guisson@gmail.com>                               *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "kbucketentry.h"
#include <util/functions.h>
#include <util/log.h>

namespace dht
{
KBucketEntry::KBucketEntry()
{
    last_responded = bt::CurrentTime();
    failed_queries = 0;
    questionable_pings = 0;
}

KBucketEntry::KBucketEntry(const net::Address &addr, const Key &id)
    : addr(addr)
    , node_id(id)
{
    last_responded = bt::CurrentTime();
    failed_queries = 0;
    questionable_pings = 0;
}

KBucketEntry::KBucketEntry(const KBucketEntry &other)
    : addr(other.addr)
    , node_id(other.node_id)
    , last_responded(other.last_responded)
    , failed_queries(other.failed_queries)
    , questionable_pings(other.questionable_pings)
{
}

KBucketEntry::~KBucketEntry()
{
}

KBucketEntry &KBucketEntry::operator=(const KBucketEntry &other)
{
    addr = other.addr;
    node_id = other.node_id;
    last_responded = other.last_responded;
    failed_queries = other.failed_queries;
    questionable_pings = other.questionable_pings;
    return *this;
}

bool KBucketEntry::operator==(const KBucketEntry &entry) const
{
    return addr == entry.addr && node_id == entry.node_id;
}

bool KBucketEntry::isGood() const
{
    if (bt::CurrentTime() - last_responded > 15 * 60 * 1000)
        return false;
    else
        return true;
}

bool KBucketEntry::isQuestionable() const
{
    if (bt::CurrentTime() - last_responded > 15 * 60 * 1000)
        return true;
    else
        return false;
}

bool KBucketEntry::isBad() const
{
    if (isGood())
        return false;

    return failed_queries > 2 || questionable_pings > 2;
}

void KBucketEntry::hasResponded()
{
    last_responded = bt::CurrentTime();
    failed_queries = 0; // reset failed queries
    questionable_pings = 0;
}

bool KBucketEntry::operator<(const dht::KBucketEntry &entry) const
{
    return node_id < entry.node_id;
}

}
