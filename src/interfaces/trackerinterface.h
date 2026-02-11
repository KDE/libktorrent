/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BT_TRACKERINTERFACE_H
#define BT_TRACKERINTERFACE_H

#include <QDateTime>
#include <QUrl>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
enum TrackerStatus {
    TRACKER_OK,
    TRACKER_ANNOUNCING,
    TRACKER_ERROR,
    TRACKER_IDLE,
};

/*!
 * \brief Interface for trackers to be used in plugins.
 */
class KTORRENT_EXPORT TrackerInterface
{
public:
    TrackerInterface(const QUrl &url);
    virtual ~TrackerInterface();

    //! See if a start request succeeded
    [[nodiscard]] bool isStarted() const
    {
        return started;
    }

    //! get the tracker url
    [[nodiscard]] QUrl trackerURL() const
    {
        return url;
    }

    //! Get the tracker status
    [[nodiscard]] TrackerStatus trackerStatus() const
    {
        return status;
    }

    //! Get a string of the current tracker status
    [[nodiscard]] QString trackerStatusString() const;

    //! Is tracker timed out
    [[nodiscard]] bool timeOut() const
    {
        return time_out;
    }

    //! Is there any warnings
    [[nodiscard]] bool hasWarning() const
    {
        return !warning.isEmpty();
    }

    /*!
     * Get the update interval in ms
     * \return interval
     */
    [[nodiscard]] Uint32 getInterval() const
    {
        return interval;
    }

    //! Set the interval
    void setInterval(Uint32 i)
    {
        interval = i;
    }

    //! Get the number of seeders
    [[nodiscard]] int getNumSeeders() const
    {
        return seeders;
    }

    //! Get the number of leechers
    [[nodiscard]] int getNumLeechers() const
    {
        return leechers;
    }

    //! Get the number of times the torrent was downloaded
    [[nodiscard]] int getTotalTimesDownloaded() const
    {
        return total_downloaded;
    }

    //! Enable or disable the tracker
    void setEnabled(bool on)
    {
        enabled = on;
    }

    //! Is the tracker enabled
    [[nodiscard]] bool isEnabled() const
    {
        return enabled;
    }

    //! Get the time in seconds to the next tracker update
    [[nodiscard]] Uint32 timeToNextUpdate() const;

    //! Reset the tracker
    virtual void reset();

protected:
    QUrl url;
    Uint32 interval;
    int seeders;
    int leechers;
    int total_downloaded;
    bool enabled;
    TrackerStatus status;
    bool time_out;
    QDateTime request_time;
    QString error;
    QString warning;
    bool started;
};

}

#endif // BT_TRACKERINTERFACE_H
