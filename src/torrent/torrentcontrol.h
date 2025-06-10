/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2005 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTTORRENTCONTROL_H
#define BTTORRENTCONTROL_H

#include <QDateTime>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>

#include "globals.h"
#include "torrent.h"
#include <interfaces/torrentinterface.h>
#include <interfaces/trackerslist.h>
#include <ktorrent_export.h>
#include <util/timer.h>

class QString;
class KJob;

namespace bt
{
class StatsFile;
class Choker;
class PeerSourceManager;
class ChunkManager;
class PeerManager;
class Downloader;
class Uploader;
class Peer;
class BitSet;
class QueueManagerInterface;
class TimeEstimator;
class WaitJob;
class MonitorInterface;
class ChunkSelectorFactoryInterface;
class CacheFactory;
class JobQueue;
class DataCheckerJob;

/*!
 * \author Joris Guisson
 * \brief Controls just about everything for a single torrent.
 *
 * This is the interface which any user gets to deal with.
 * This class controls the uploading, downloading, choking,
 * updating the tracker and chunk management.
 */
class KTORRENT_EXPORT TorrentControl : public TorrentInterface, public FilePriorityListener
{
    Q_OBJECT
public:
    TorrentControl();
    ~TorrentControl() override;

    //! Get the Torrent.
    const Torrent &getTorrent() const
    {
        return *tor;
    }

    /*!
     * Initialize the TorrentControl.
     * \param qman The QueueManager
     * \param data The data of the torrent
     * \param tmpdir The directory to store temporary data
     * \param datadir The directory to store the actual file(s)
     *      (only used the first time we load a torrent)
     * \throw Error when something goes wrong
     */
    void init(QueueManagerInterface *qman, const QByteArray &data, const QString &tmpdir, const QString &datadir);

    //! Tell the TorrentControl obj to preallocate diskspace in the next update
    void setPreallocateDiskSpace(bool pa)
    {
        prealloc = pa;
    }

    //! Test if the torrent has existing files, only works the first time a torrent is loaded
    bool hasExistingFiles() const;

    const BitSet &downloadedChunksBitSet() const override;
    const BitSet &availableChunksBitSet() const override;
    const BitSet &excludedChunksBitSet() const override;
    const BitSet &onlySeedChunksBitSet() const override;
    bool changeTorDir(const QString &new_dir) override;
    bool changeOutputDir(const QString &new_dir, ChangeOutputOptions flags) override;
    void rollback() override;
    void setDisplayName(const QString &n) override;
    TrackersList *getTrackersList() override;
    const TrackersList *getTrackersList() const override;
    QString getDataDir() const override
    {
        return outputdir;
    }
    QString getTorDir() const override
    {
        return tordir;
    }
    void setMonitor(MonitorInterface *tmo) override;
    Uint32 getRunningTimeDL() const override;
    Uint32 getRunningTimeUL() const override;
    Uint32 getNumFiles() const override;
    TorrentFileInterface &getTorrentFile(Uint32 index) override;
    const TorrentFileInterface &getTorrentFile(Uint32 index) const override;
    bool moveTorrentFiles(const QMap<TorrentFileInterface *, QString> &files) override;
    void recreateMissingFiles() override;
    void dndMissingFiles() override;
    TorrentFileStream::Ptr createTorrentFileStream(bt::Uint32 index, bool streaming_mode, QObject *parent) override;
    void addPeerSource(PeerSource *ps) override;
    void removePeerSource(PeerSource *ps) override;
    Uint32 getNumWebSeeds() const override;
    const WebSeedInterface *getWebSeed(Uint32 i) const override;
    WebSeedInterface *getWebSeed(Uint32 i) override;
    bool addWebSeed(const QUrl &url) override;
    bool removeWebSeed(const QUrl &url) override;
    bool readyForPreview() const override;
    bool isMultimedia() const override;
    void markExistingFilesAsDownloaded() override;
    int getPriority() const override
    {
        return istats.priority;
    }
    void setPriority(int p) override;
    bool overMaxRatio() override;
    void setMaxShareRatio(float ratio) override;
    float getMaxShareRatio() const override
    {
        return stats.max_share_ratio;
    }
    bool overMaxSeedTime() override;
    void setMaxSeedTime(float hours) override;
    float getMaxSeedTime() const override
    {
        return stats.max_seed_time;
    }
    void setAllowedToStart(bool on) override;
    void setQueued(bool queued) override;
    void setChunkSelector(std::unique_ptr<ChunkSelectorInterface> csel) override;
    void networkUp() override;
    bool announceAllowed() override;
    Job *startDataCheck(bool auto_import, bt::Uint32 from, bt::Uint32 to) override;
    bool hasMissingFiles(QStringList &sl) override;
    bool isStorageMounted(QStringList &missing) override;
    Uint32 getNumDHTNodes() const override;
    const DHTNode &getDHTNode(Uint32 i) const override;
    void deleteDataFiles() override;
    const bt::PeerID &getOwnPeerID() const override;
    QString getComments() const override;
    const JobQueue *getJobQueue() const override
    {
        return job_queue;
    }
    bool isFeatureEnabled(TorrentFeature tf) override;
    void setFeatureEnabled(TorrentFeature tf, bool on) override;
    bool checkDiskSpace(bool emit_sig = true) override;
    void setTrafficLimits(Uint32 up, Uint32 down) override;
    void getTrafficLimits(Uint32 &up, Uint32 &down) override;
    void setAssuredSpeeds(Uint32 up, Uint32 down) override;
    void getAssuredSpeeds(Uint32 &up, Uint32 &down) override;
    const SHA1Hash &getInfoHash() const override;
    void setUserModifiedFileName(const QString &n) override;
    int getETA() override;
    void setMoveWhenCompletedDir(const QString &dir) override
    {
        completed_dir = dir;
        saveStats();
    }
    QString getMoveWhenCompletedDir() const override
    {
        return completed_dir;
    }
    void setSuperSeeding(bool on) override;

    //! Create all the necessary files
    void createFiles();

    //! Get the PeerManager
    const PeerManager *getPeerMgr() const;

    /*!
     * Set a custom chunk selector factory (needs to be done for init is called)
     * Note: TorrentControl does not take ownership
     */
    void setChunkSelectorFactory(ChunkSelectorFactoryInterface *csfi);

    //! Set a custom Cache factory
    void setCacheFactory(std::unique_ptr<CacheFactory> cf);

    //! Get time in msec since the last Stats file save on disk
    TimeStamp getStatsSyncElapsedTime()
    {
        return stats_save_timer.getElapsedSinceUpdate();
    }

public:
    /*!
     * Update the object, should be called periodically.
     */
    void update() override;

    /*!
     * Pause the torrent.
     */
    void pause() override;

    /*!
     * Unpause the torrent.
     */
    void unpause() override;

    /*!
     * Start the download of the torrent.
     */
    void start() override;

    /*!
     * Stop the download, closes all connections.
     * \param wjob WaitJob to wait at exit for the completion of stopped requests
     */
    void stop(WaitJob *wjob = nullptr) override;

    /*!
     * Update the tracker, this should normally handled internally.
     * We leave it public so that the user can do a manual announce.
     */
    void updateTracker() override;

    /*!
     * Scrape the tracker.
     * */
    void scrapeTracker() override;

    /*!
     * A scrape has finished on the tracker.
     * */
    void trackerScrapeDone();

    void afterRename();

    /*!
     * Enable or disable data check upon completion
     * \param on
     */
    static void setDataCheckWhenCompleted(bool on)
    {
        completed_datacheck = on;
    }

    /*!
     * Set the minimum amount of diskspace in MB. When there is less then this free, torrents will be stopped.
     * \param m
     */
    static void setMinimumDiskSpace(Uint32 m)
    {
        min_diskspace = m;
    }

protected:
    //! Called when a data check is finished by DataCheckerJob
    void afterDataCheck(DataCheckerJob *job, const BitSet &result);
    void beforeDataCheck();
    void preallocFinished(const QString &error, bool completed);
    void allJobsDone();
    bool preallocate();

private:
    void onNewPeer(Peer *p);
    void onPeerRemoved(Peer *p);
    void doChoking();
    void onIOError(const QString &msg);
    //! Update the stats of the torrent.
    void updateStats();
    void corrupted(Uint32 chunk);
    void moveDataFilesFinished(KJob *j);
    void moveDataFilesWithMapFinished(KJob *j);
    void downloaded(Uint32 chunk);
    void moveToCompletedDir();
    void emitFinished();

    void updateTracker(const QString &ev, bool last_succes = true);
    void updateStatus() override;
    void saveStats();
    void loadStats();
    void loadOutputDir();
    void getSeederInfo(Uint32 &total, Uint32 &connected_to) const;
    void getLeecherInfo(Uint32 &total, Uint32 &connected_to) const;
    void continueStart();
    void handleError(const QString &err) override;
    void initInternal(QueueManagerInterface *qman, const QString &tmpdir, const QString &ddir);
    void checkExisting(QueueManagerInterface *qman);
    void setupDirs(const QString &tmpdir, const QString &ddir);
    void setupStats();
    void setupData();
    void setUploadProps(Uint32 limit, Uint32 rate);
    void setDownloadProps(Uint32 limit, Uint32 rate);
    void downloadPriorityChanged(TorrentFile *tf, Priority newpriority, Priority oldpriority) override;
    void updateRunningTimes();

Q_SIGNALS:
    void dataCheckFinished();

private:
    JobQueue *job_queue; // lifetime managed by QObject
    QueueManagerInterface *m_qman;
    std::unique_ptr<Choker> choke;
    std::unique_ptr<Downloader> downloader;
    std::unique_ptr<Uploader> uploader;
    std::unique_ptr<ChunkManager> cman;
    std::unique_ptr<PeerManager> pman;
    std::unique_ptr<PeerSourceManager> psman;
    std::unique_ptr<Torrent> tor;
    std::unique_ptr<TimeEstimator> m_eta;
    MonitorInterface *tmon;
    std::unique_ptr<CacheFactory> cache_factory;
    QString move_data_files_destination_path;
    Timer choker_update_timer;
    Timer stats_save_timer;
    Timer stalled_timer;
    Timer wanted_update_timer;
    QString tordir;
    QString old_tordir;
    QString outputdir;
    QString error_msg;
    QString completed_dir;
    bool prealloc;
    TimeStamp last_diskspace_check;
    bool loading_stats = false;

    struct InternalStats {
        QDateTime time_started_dl;
        QDateTime time_started_ul;
        Uint32 running_time_dl;
        Uint32 running_time_ul;
        Uint64 prev_bytes_dl;
        Uint64 prev_bytes_ul;
        Uint64 session_bytes_uploaded;
        bool io_error;
        bool custom_output_name;
        Uint16 port;
        int priority;
        bool dht_on;
        bool diskspace_warning_emitted;
    };

    // by default no torrent limits
    Uint32 upload_gid = 0; // group ID for upload
    Uint32 upload_limit = 0;
    Uint32 download_gid = 0; // group ID for download
    Uint32 download_limit = 0;

    Uint32 assured_download_speed = 0;
    Uint32 assured_upload_speed = 0;

    InternalStats istats;
    std::unique_ptr<StatsFile> stats_file;

    TorrentFileStream::WPtr stream;

    static bool completed_datacheck;
    static Uint32 min_diskspace;

    friend class DataCheckerJob;
    friend class PreallocationJob;
    friend class JobQueue;
};

}

#endif
