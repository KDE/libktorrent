/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCACHEFACTORY_H
#define BTCACHEFACTORY_H

#include <QString>
#include <ktorrent_export.h>

namespace bt
{
class Cache;
class Torrent;

/*!
 * \brief Interface to implement for creating custom Cache objects.
 *
 * If you want a custom Cache you need to derive from this class
 * and implement the create method to create your own custom Caches.
 * \author Joris Guisson
 */
class KTORRENT_EXPORT CacheFactory
{
public:
    CacheFactory();
    virtual ~CacheFactory();

    /*!
     * Create a custom Cache
     * \param tor The Torrent
     * \param tmpdir The temporary directory (should be used to store information about the torrent)
     * \param datadir The data directory, where to store the data
     * \return
     */
    virtual Cache *create(Torrent &tor, const QString &tmpdir, const QString &datadir) = 0;
};

}

#endif
