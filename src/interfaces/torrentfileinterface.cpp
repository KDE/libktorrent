/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "torrentfileinterface.h"
#include <util/fileops.h>
#include <util/functions.h>

namespace bt
{
TorrentFileInterface::TorrentFileInterface(Uint32 index, const QString &path, Uint64 size)
    : index(index)
    , first_chunk(0)
    , last_chunk(0)
    , num_chunks_downloaded(0)
    , size(size)
    , first_chunk_off(0)
    , last_chunk_size(0)

    , preexisting(false)
    , emit_status_changed(true)
    , preview(false)
    , filetype(UNKNOWN)
    , priority(NORMAL_PRIORITY)

    , path(path)
{
}

TorrentFileInterface::~TorrentFileInterface()
{
}

float TorrentFileInterface::getDownloadPercentage() const
{
    Uint32 num = last_chunk - first_chunk + 1;
    return 100.0f * (float)num_chunks_downloaded / num;
}

void TorrentFileInterface::setUnencodedPath(const QList<QByteArray> up)
{
    unencoded_path = up;
}

QString TorrentFileInterface::getMountPoint() const
{
    if (!bt::Exists(path_on_disk))
        return QString();

    if (mount_point.isEmpty())
        mount_point = bt::MountPoint(path_on_disk);

    return mount_point;
}

}

#include "moc_torrentfileinterface.cpp"
