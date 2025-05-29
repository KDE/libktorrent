/*
    SPDX-FileCopyrightText: 2005 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTTRACKERSLIST_H
#define BTTRACKERSLIST_H

#include <QUrl>
#include <ktorrent_export.h>

namespace bt
{
struct TrackerTier;
class TrackerInterface;

struct TrackersStatusInfo {
    int trackers_count;
    int errors;
    int timeout_errors;
    int warnings;
};

/*!
 * \author Ivan Vasić <ivasic@gmail.com>
 *
 * This interface is used to provide access to AnnounceList object which holds a list of available trackers for a torrent.
 */
class KTORRENT_EXPORT TrackersList
{
public:
    TrackersList();
    virtual ~TrackersList();

    /*!
     * Get the current tracker (for non private torrents this returns 0, seeing that
     * all trackers are used at the same time)
     */
    virtual TrackerInterface *getCurrentTracker() const = 0;

    /*!
     * Sets the current tracker and does the announce. For non private torrents, this
     * does nothing.
     * \param t The Tracker
     */
    virtual void setCurrentTracker(TrackerInterface *t) = 0;

    /*!
     * Sets the current tracker and does the announce. For non private torrents, this
     * does nothing.
     * \param url Url of the tracker
     */
    virtual void setCurrentTracker(const QUrl &url) = 0;

    /*!
     * Gets a list of all available trackers.
     */
    virtual QList<TrackerInterface *> getTrackers() = 0;

    /*!
     * Adds a tracker URL to the list.
     * \param url The URL
     * \param custom Is it a custom tracker
     * \param tier Which tier (or priority) the tracker has, tier 1 are
     * the main trackers, tier 2 are backups ...
     * \return The Tracker
     */
    virtual TrackerInterface *addTracker(const QUrl &url, bool custom = true, int tier = 1) = 0;

    /*!
     * Removes the tracker from the list.
     * \param t The Tracker
     */
    virtual bool removeTracker(TrackerInterface *t) = 0;

    /*!
     * Removes the tracker from the list.
     * \param url The tracker url
     */
    virtual bool removeTracker(const QUrl &url) = 0;

    /*!
     * Return true if a tracker can be removed
     * \param t The tracker
     */
    virtual bool canRemoveTracker(TrackerInterface *t) = 0;

    /*!
     * Restores the default tracker and does the announce.
     */
    virtual void restoreDefault() = 0;

    /*!
     * Set a tracker enabled or not
     */
    virtual void setTrackerEnabled(const QUrl &url, bool on) = 0;

    /*!
     * Merge an other tracker list.
     * \param first The first TrackerTier
     */
    void merge(const bt::TrackerTier *first);

    /*!
     * Returns true if no tracker is reachable
     */
    virtual bool noTrackersReachable() const = 0;

    /*!
     * Returns true if any tracker has time out error
     */
    virtual TrackersStatusInfo getTrackersStatusInfo() const = 0;
};

}

#endif
