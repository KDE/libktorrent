/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_MAGNETLINK_H
#define BT_MAGNETLINK_H

#include <QUrl>
#include <ktorrent_export.h>
#include <util/infohash.h>

namespace bt
{
/*!
    MagnetLink class
    magnet links have the format:
    magnet:?xt=urn:btih:info_hash&dn=name&tr=tracker-url[,tracker-url...]
    note: a comma-separated list will not work with other clients likely
    optional parameters are
    to=torrent-file-url (need not be valid)
    xs=torrent-file-url (eXact Source - need not be valid)
    pt=path-to-download-in-torrent
*/
class KTORRENT_EXPORT MagnetLink
{
    friend class MagnetDownloader;

public:
    MagnetLink();
    MagnetLink(const MagnetLink &mlink);
    MagnetLink(const QUrl &mlink);
    MagnetLink(const QString &mlink);
    ~MagnetLink();

    //! Assignment operator
    MagnetLink &operator=(const MagnetLink &mlink);

    //! Equality operator
    bool operator==(const MagnetLink &mlink) const;

    //! Is this a valid magnet link
    bool isValid() const
    {
        return !magnet_string.isEmpty();
    }

    //! Convert it to a string
    QString toString() const
    {
        return magnet_string;
    }

    //! Get the display name (can be empty)
    QString displayName() const
    {
        return name;
    }

    //! Get the path of addressed file(s) inside the torrent
    QString subPath() const
    {
        return path;
    }

    //! Get the torrent URL (can be empty)
    QString torrent() const
    {
        return torrent_url;
    }

    //! Get all possible trackers (can be empty)
    QList<QUrl> trackers() const
    {
        return tracker_urls;
    }

    //! Get the info hash
    const InfoHash &infoHash() const
    {
        return info_hash;
    }

private:
    void parse(const QUrl &mlink);
    Uint8 charToHex(const QChar &ch);
    QString base32ToHexString(const QString &s);

private:
    QString magnet_string;
    InfoHash info_hash;
    QString torrent_url;
    QList<QUrl> tracker_urls;
    QString path;
    QString name;
};

}

#endif // BT_MAGNETLINK_H
