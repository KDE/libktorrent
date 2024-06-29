/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "connectionlimit.h"
#include <util/functions.h>

namespace bt
{
static bt::Uint32 SystemConnectionLimit()
{
#ifndef Q_OS_WIN
    return bt::MaxOpenFiles() - 50; // leave about 50 free for regular files
#else
    return 9999; // there isn't a real limit on windows
#endif
}

ConnectionLimit::ConnectionLimit()
    : global_limit(SystemConnectionLimit())
    , global_total(0)
    , torrent_limit(0)
{
}

ConnectionLimit::~ConnectionLimit()
{
}

void ConnectionLimit::setLimits(Uint32 global_limit, Uint32 torrent_limit)
{
    this->global_limit = global_limit;
    this->torrent_limit = torrent_limit;
    if (this->global_limit > SystemConnectionLimit())
        this->global_limit = SystemConnectionLimit();
}

ConnectionLimit::Token::Ptr ConnectionLimit::acquire(const SHA1Hash &hash)
{
    if (global_limit != 0 && global_total >= global_limit)
        return Token::Ptr();

    QMap<SHA1Hash, bt::Uint32>::iterator i = torrent_totals.find(hash);
    if (i == torrent_totals.end()) {
        torrent_totals[hash] = 1;
        global_total++;
        return Token::Ptr(new Token(*this, hash));
    } else if (torrent_limit == 0 || i.value() < torrent_limit) {
        i.value()++;
        global_total++;
        return Token::Ptr(new Token(*this, hash));
    }

    return Token::Ptr();
}

void ConnectionLimit::release(const ConnectionLimit::Token &token)
{
    QMap<SHA1Hash, bt::Uint32>::iterator i = torrent_totals.find(token.infoHash());
    if (i != torrent_totals.end()) {
        if (i.value() > 0)
            i.value()--;

        // erase when torrent has no tokens left
        if (i.value() == 0)
            torrent_totals.erase(i);

        if (global_total > 0)
            global_total--;
    }
}

ConnectionLimit::Token::Token(ConnectionLimit &limit, const SHA1Hash &hash)
    : limit(limit)
    , hash(hash)
{
}

ConnectionLimit::Token::~Token()
{
    limit.release(*this);
}

}
