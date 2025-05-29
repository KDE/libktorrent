/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTTRACKER_H
#define BTTRACKER_H

#include <QTimer>
#include <QUrl>

#include <interfaces/peersource.h>
#include <interfaces/trackerinterface.h>
#include <ktorrent_export.h>
#include <peer/peerid.h>
#include <util/sha1hash.h>

class QUrl;

namespace bt
{
/*!
    Interface used by the Tracker to obtain the data it needs to know
    when announcing.
*/
class KTORRENT_EXPORT TrackerDataSource
{
public:
    virtual ~TrackerDataSource()
    {
    }

    virtual Uint64 bytesDownloaded() const = 0;
    virtual Uint64 bytesUploaded() const = 0;
    virtual Uint64 bytesLeft() const = 0;
    virtual const SHA1Hash &infoHash() const = 0;
    virtual bool isPartialSeed() const = 0;
};

/*!
 * Base class for all tracker classes.
 */
class KTORRENT_EXPORT Tracker : public PeerSource, public TrackerInterface
{
    Q_OBJECT
public:
    Tracker(const QUrl &url, TrackerDataSource *tds, const PeerID &id, int tier);
    ~Tracker() override;

    /*!
     * Set the custom IP
     * \param str
     */
    static void setCustomIP(const QString &str);

    //! get the tracker url
    QUrl trackerURL() const
    {
        return url;
    }

    /*!
     * Delete the tracker in ms milliseconds, or when the stopDone signal is emitted.
     * \param ms Number of ms to wait
     */
    void timedDelete(int ms);

    /*!
     * Get the number of failed attempts to reach a tracker.
     * \return The number of failed attempts
     */
    virtual Uint32 failureCount() const = 0;

    /*!
     * Do a tracker scrape to get more accurate stats about a torrent.
     * Does nothing if the tracker does not support this.
     */
    virtual void scrape() = 0;

    //! Get the trackers tier
    int getTier() const
    {
        return tier;
    }

    //! Get the custom ip to use, null if none is set
    static QString getCustomIP();

    //! Handle a failure
    void handleFailure();

protected:
    //! Reset the tracker stats
    void resetTrackerStats();

    //! Calculates the bytes downloaded to send with the request
    Uint64 bytesDownloaded() const;

    //! Calculates the bytes uploaded to send with the request
    Uint64 bytesUploaded() const;

    //! Emit the failure signal, and set the error
    void failed(const QString &err);

public:
    void manualUpdate() override = 0;

Q_SIGNALS:
    /*!
     * Emitted when an error happens.
     * \param failure_reason The reason why we couldn't reach the tracker
     */
    void requestFailed(const QString &failure_reason);

    /*!
     * Emitted when a stop is done.
     */
    void stopDone();

    /*!
     * Emitted when a request to the tracker succeeded
     */
    void requestOK();

    /*!
     * A request to the tracker has been started.
     */
    void requestPending();

    /*!
     * Emitted when a scrape has finished
     * */
    void scrapeDone();

protected:
    int tier;
    PeerID peer_id;
    TrackerDataSource *tds;
    Uint32 key;
    QTimer reannounce_timer;
    Uint64 bytes_downloaded_at_start;
    Uint64 bytes_uploaded_at_start;
};
}

#endif
