/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPEERID_H
#define BTPEERID_H

#include <QString>
#include <ktorrent_export.h>

namespace bt
{
/*!
\author Joris Guisson
*/
class KTORRENT_EXPORT PeerID
{
    char id[20];
    QString client_name;

public:
    PeerID();
    PeerID(const char *pid);
    PeerID(const PeerID &pid);
    virtual ~PeerID();

    PeerID &operator=(const PeerID &pid);

    const char *data() const
    {
        return id;
    }

    QString toString() const;

    /*!
     * Interprets the PeerID to figure out which client it is.
     * \author Ivan + Joris
     * \return The name of the client
     */
    QString identifyClient() const;

    friend bool operator==(const PeerID &a, const PeerID &b);
    friend bool operator!=(const PeerID &a, const PeerID &b);
    friend bool operator<(const PeerID &a, const PeerID &b);
};

}

#endif
