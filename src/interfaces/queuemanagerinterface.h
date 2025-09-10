/*
    SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTQUEUEMANAGERINTERFACE_H
#define BTQUEUEMANAGERINTERFACE_H

#include <ktorrent_export.h>

namespace bt
{
class SHA1Hash;
class TorrentControl;
struct TrackerTier;

/*!
 * \headerfile interfaces/queuemanagerinterface.h
 * \brief Interface that should own all TorrentInterface's, and is used to ensure a torrent isn't loaded twice.
 */
class KTORRENT_EXPORT QueueManagerInterface
{
    static bool qm_enabled;

public:
    QueueManagerInterface();
    virtual ~QueueManagerInterface();

    /*!
     * See if we already loaded a torrent.
     * \param ih The info hash of a torrent
     * \return true if we do, false if we don't
     */
    [[nodiscard]] virtual bool alreadyLoaded(const SHA1Hash &ih) const = 0;

    /*!
     * Merge announce lists to a torrent
     * \param ih The info_hash of the torrent to merge to
     * \param trk First tier of trackers
     */
    virtual void mergeAnnounceList(const SHA1Hash &ih, const TrackerTier *trk) = 0;

    /*!
     * Disable or enable the QM
     * \param on
     */
    static void setQueueManagerEnabled(bool on);

    /*!
     * Requested by each TorrentControl during its update to
     * get permission on saving Stats file to disk. May be
     * overriden to balance I/O operations.
     * \param tc Pointer to TorrentControl instance
     * \return true if file save is permitted, false otherwise
     */

    virtual bool permitStatsSync(TorrentControl *tc);

    static bool enabled()
    {
        return qm_enabled;
    }
};

}

#endif
