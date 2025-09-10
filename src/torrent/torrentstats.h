/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_TORRENTSTATS_H
#define BT_TORRENTSTATS_H

#include <QDateTime>
#include <QString>
#include <ktorrent_export.h>
#include <util/constants.h>

#if defined ERROR
#undef ERROR
#endif

namespace bt
{
enum TorrentStatus {
    NOT_STARTED,
    SEEDING_COMPLETE,
    DOWNLOAD_COMPLETE,
    SEEDING,
    DOWNLOADING,
    STALLED,
    STOPPED,
    ALLOCATING_DISKSPACE,
    ERROR,
    QUEUED,
    CHECKING_DATA,
    NO_SPACE_LEFT,
    PAUSED,
    SUPERSEEDING,
    INVALID_STATUS,
};

/*!
 * \headerfile torrent/torrentstats.h
 * \brief Stores statistics about a torrent.
 */
struct KTORRENT_EXPORT TorrentStats {
    //! Error message for the user
    QString error_msg;
    //! QDateTime when the torrent was added
    QDateTime time_added;
    //! Name of the torrent
    QString torrent_name;
    //! Path of the dir or file where the data will get saved
    QString output_path;
    //! The number of bytes imported (igore these for average speed)
    Uint64 imported_bytes;
    //! Total number of bytes downloaded.
    Uint64 bytes_downloaded;
    //! Total number of bytes uploaded.
    Uint64 bytes_uploaded;
    //! The number of bytes left (gets sent to the tracker)
    Uint64 bytes_left;
    //! The number of bytes left to download (bytes_left - excluded bytes)
    Uint64 bytes_left_to_download;
    //! total number of bytes in torrent
    Uint64 total_bytes;
    //! The total number of bytes which need to be downloaded
    Uint64 total_bytes_to_download;
    //! The download rate in bytes per sec
    Uint32 download_rate;
    //! The upload rate in bytes per sec
    Uint32 upload_rate;
    //! The number of peers we are connected to
    Uint32 num_peers;
    //! The number of chunks we are currently downloading
    Uint32 num_chunks_downloading;
    //! The total number of chunks
    Uint32 total_chunks;
    //! The number of chunks which have been downloaded
    Uint32 num_chunks_downloaded;
    //! Get the number of chunks which have been excluded
    Uint32 num_chunks_excluded;
    //! Get the number of chunks left
    Uint32 num_chunks_left;
    //! Size of each chunk
    Uint32 chunk_size;
    //! Total seeders in swarm
    Uint32 seeders_total;
    //! Num seeders connected to
    Uint32 seeders_connected_to;
    //! Total leechers in swarm
    Uint32 leechers_total;
    //! Num leechers connected to
    Uint32 leechers_connected_to;
    //! Status of the download
    TorrentStatus status;
    //! The number of bytes downloaded in this session
    Uint64 session_bytes_downloaded;
    //! The number of bytes uploaded in this session
    Uint64 session_bytes_uploaded;
    //! See if we are running
    bool running;
    //! See if the torrent has been started
    bool started;
    //! Whether or not the torrent is queued
    bool queued;
    //! See if we are allowed to startup this torrent automatically.
    bool autostart;
    //! See if the torrent is stopped by error
    bool stopped_by_error;
    //! See if the download is completed
    bool completed;
    //! See if this torrent is paused
    bool paused;
    //! Set to true if torrent was stopped due to reaching max share ration or max seed time
    bool auto_stopped;
    //! Set to true if superseeding is enabled
    bool superseeding;
    //! Whether or not the QM can start this torrent
    bool qm_can_start;
    //! See if we have a multi file torrent
    bool multi_file_torrent;
    //! Private torrent (i.e. no use of DHT)
    bool priv_torrent;
    //! Maximum share ratio
    float max_share_ratio;
    //! Maximum seed time in hours
    float max_seed_time;
    //! Number of corrupted chunks found since the last check
    Uint32 num_corrupted_chunks;
    //! TimeStamp when we last saw download activity
    TimeStamp last_download_activity_time;
    //! TimeStamp when we last saw upload activity
    TimeStamp last_upload_activity_time;

    TorrentStats();

    //! Calculate the share ratio
    [[nodiscard]] float shareRatio() const;

    //! Are we over the max share ratio
    [[nodiscard]] bool overMaxRatio() const;

    //! Convert the status into a human readable string
    [[nodiscard]] QString statusToString() const;
};
}

#endif // BT_TORRENTSTATS_H
