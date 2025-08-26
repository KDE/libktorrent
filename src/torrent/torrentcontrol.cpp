/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2005 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "torrentcontrol.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <KIO/CopyJob>
#include <KLocalizedString>

#include "choker.h"
#include "globals.h"
#include "jobqueue.h"
#include "peersourcemanager.h"
#include "server.h"
#include "statsfile.h"
#include "timeestimator.h"
#include "torrent.h"
#include "torrentfile.h"
#include "torrentfilestream.h"
#include "uploader.h"
#include <datachecker/datacheckerjob.h>
#include <datachecker/datacheckerthread.h>
#include <datachecker/multidatachecker.h>
#include <datachecker/singledatachecker.h>
#include <dht/dhtbase.h>
#include <diskio/cache.h>
#include <diskio/chunkmanager.h>
#include <diskio/movedatafilesjob.h>
#include <diskio/preallocationjob.h>
#include <diskio/preallocationthread.h>
#include <download/downloader.h>
#include <download/webseed.h>
#include <interfaces/cachefactory.h>
#include <interfaces/chunkselectorinterface.h>
#include <interfaces/monitorinterface.h>
#include <interfaces/queuemanagerinterface.h>
#include <interfaces/trackerslist.h>
#include <net/socketmonitor.h>
#include <peer/peer.h>
#include <peer/peerdownloader.h>
#include <peer/peermanager.h>
#include <util/bitset.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/waitjob.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
bool TorrentControl::completed_datacheck = false;
Uint32 TorrentControl::min_diskspace = 100;

TorrentControl::TorrentControl()
    : job_queue(new JobQueue(this))
    , m_qman(nullptr)
    , m_eta(std::make_unique<TimeEstimator>(this))
    , tmon(nullptr)
    , prealloc(false)
    , last_diskspace_check(bt::CurrentTime())
{
    istats.session_bytes_uploaded = 0;

    istats.running_time_dl = istats.running_time_ul = 0;
    istats.prev_bytes_dl = 0;
    istats.prev_bytes_ul = 0;
    istats.io_error = false;
    istats.priority = 0;
    istats.custom_output_name = false;
    istats.diskspace_warning_emitted = false;
    istats.dht_on = false;
    updateStats();
}

TorrentControl::~TorrentControl()
{
    if (stats.running) {
        // block all signals to prevent crash at exit
        blockSignals(true);
        stop(nullptr);
    }

    if (tmon)
        tmon->destroyed();

    if (downloader)
        downloader->saveWebSeeds(tordir + "webseeds"_L1);
}

void TorrentControl::update()
{
    UpdateCurrentTime();

    if (istats.io_error) {
        stop();
        Q_EMIT stoppedByError(this, stats.error_msg);
        return;
    }

    if (stats.paused) {
        stalled_timer.update();
        pman->update();
        updateStatus();
        updateStats();
        return;
    }

    if (prealloc && preallocate())
        return;

    try {
        // first update peermanager
        pman->update();
        bool comp = stats.completed;

        // then the downloader and uploader
        uploader->update();
        downloader->update();

        // helper var, check if needed to move completed files somewhere
        bool moveCompleted = false;
        bool checkOnCompletion = false;

        stats.completed = cman->completed();
        if (stats.completed && !comp) {
            pman->killSeeders();
            QDateTime now = QDateTime::currentDateTime();
            istats.running_time_dl += istats.time_started_dl.secsTo(now);
            updateStatus();
            updateStats();

            // download has just been completed
            // only sent completed to tracker when we have all chunks (so no excluded chunks)
            if (cman->haveAllChunks())
                psman->completed();
            pman->setPartialSeed(!cman->haveAllChunks() && cman->chunksLeft() == 0);

            Q_EMIT finished(this);

            // Move completed download to specified directory if needed
            moveCompleted = !completed_dir.isEmpty();

            // See if we need to do a data check
            if (completed_datacheck)
                checkOnCompletion = true;
        } else if (!stats.completed && comp) {
            // restart download if necesarry
            // when user selects that files which were previously excluded,
            // should now be downloaded
            if (!psman->isStarted())
                psman->start();
            else
                psman->manualUpdate();
            istats.time_started_dl = QDateTime::currentDateTime();
            // Tell QM to redo queue
            Q_EMIT updateQueue();
            if (stats.superseeding)
                pman->setSuperSeeding(false, cman->getBitSet());
        }
        updateStatus();

        if (wanted_update_timer.getElapsedSinceUpdate() >= 60 * 1000) {
            // Get a list of the chunks I have...
            BitSet wanted_chunks = cman->getBitSet();
            // or don't want...
            wanted_chunks.orBitSet(cman->getExcludedBitSet());
            wanted_chunks.orBitSet(cman->getOnlySeedBitSet());
            // and inverted it get a list of the chunks I want
            wanted_chunks.invert();
            pman->setWantedChunks(wanted_chunks);
            wanted_update_timer.update();
        }

        // we may need to update the choker
        if (choker_update_timer.getElapsedSinceUpdate() >= 10000 || pman->chokerNeedsToRun()) {
            // also get rid of seeders & uninterested when download is finished
            // no need to keep them around, but also no need to do this
            // every update, so once every 10 seconds is fine
            if (stats.completed) {
                pman->killSeeders();
            }

            doChoking();
            choker_update_timer.update();
            // a good opportunity to make sure we are not keeping to much in memory
            cman->checkMemoryUsage();
        }

        if (stats.download_rate > 100 && stats.bytes_left > 0) {
            stalled_timer.update();
            stats.last_download_activity_time = CurrentTime();
        }

        if (stats.upload_rate > 100) {
            stats.last_upload_activity_time = CurrentTime();
        }

        // to satisfy people obsessed with their share ratio
        bool save_stats = m_qman ? m_qman->permitStatsSync(this) : (stats_save_timer.getElapsedSinceUpdate() >= 5 * 60 * 1000);

        if (save_stats) {
            saveStats();
            stats_save_timer.update();
        }

        // Update DownloadCap
        updateStats();

        // do a manual update if we are stalled for more then 2 minutes
        // we do not do this for private torrents
        if (stalled_timer.getElapsedSinceUpdate() > 120000 && !stats.completed && !stats.priv_torrent) {
            Out(SYS_TRK | LOG_NOTICE) << "Stalled for too long, time to get some fresh blood" << endl;
            psman->manualUpdate();
            stalled_timer.update();
        }

        if (stats.completed && (overMaxRatio() || overMaxSeedTime())) {
            stats.auto_stopped = true;
            stop();
            Q_EMIT seedingAutoStopped(this, overMaxRatio() ? MAX_RATIO_REACHED : MAX_SEED_TIME_REACHED);
        }

        // Update diskspace if needed (every 1 min)
        if (!stats.completed && stats.running && bt::CurrentTime() - last_diskspace_check >= 60 * 1000) {
            checkDiskSpace(true);
        }

        // Emit the needDataCheck signal if needed
        if (checkOnCompletion)
            Q_EMIT needDataCheck(this);

        // Move completed files if needed:
        if (moveCompleted)
            moveToCompletedDir();
    }
#ifndef Q_OS_WIN
    catch (BusError &e) {
        Out(SYS_DIO | LOG_IMPORTANT) << "Caught SIGBUS " << endl;
        if (!e.write_operation)
            onIOError(e.toString());
        else
            onIOError(i18n("Error writing to disk, do you have enough diskspace?"));
    }
#endif
    catch (Error &e) {
        onIOError(e.toString());
    }
}

void TorrentControl::onIOError(const QString &msg)
{
    Out(SYS_DIO | LOG_IMPORTANT) << "Error : " << msg << endl;
    stats.stopped_by_error = true;
    stats.status = ERROR;
    stats.error_msg = msg;
    istats.io_error = true;
    Q_EMIT statusChanged(this);
}

void TorrentControl::pause()
{
    if (!stats.running || stats.paused)
        return;

    pman->pause();

    try {
        downloader->saveDownloads(tordir + "current_chunks"_L1);
    } catch (Error &e) {
        // print out warning in case of failure
        // it doesn't corrupt the data, so just a couple of lost chunks
        Out(SYS_GEN | LOG_NOTICE) << "Warning : " << e.toString() << endl;
    }

    downloader->pause();
    downloader->saveWebSeeds(tordir + "webseeds"_L1);
    downloader->removeAllWebSeeds();
    cman->stop();
    stats.paused = true;
    updateRunningTimes();
    saveStats();
    Q_EMIT statusChanged(this);

    Out(SYS_GEN | LOG_NOTICE) << "Paused " << tor->getNameSuggestion() << endl;
}

void TorrentControl::unpause()
{
    if (!stats.running || !stats.paused || job_queue->runningJobs())
        return;

    cman->start();

    try {
        downloader->loadDownloads(tordir + "current_chunks"_L1);
    } catch (Error &e) {
        // print out warning in case of failure
        // we can still continue the download
        Out(SYS_GEN | LOG_NOTICE) << "Warning : " << e.toString() << endl;
    }

    downloader->loadWebSeeds(tordir + "webseeds"_L1);
    pman->unpause();
    loadStats();
    istats.time_started_ul = istats.time_started_dl = QDateTime::currentDateTime();
    stats.paused = false;
    Q_EMIT statusChanged(this);
    Out(SYS_GEN | LOG_NOTICE) << "Unpaused " << tor->getNameSuggestion() << endl;
}

void TorrentControl::start()
{
    // do not start running torrents or when there is a job running
    if (stats.running || job_queue->runningJobs())
        return;

    if (stats.running && stats.paused) {
        unpause();
        return;
    }

    stats.paused = false;
    stats.stopped_by_error = false;
    istats.io_error = false;
    istats.diskspace_warning_emitted = false;
    try {
        bool ret = true;
        Q_EMIT aboutToBeStarted(this, ret);
        if (!ret)
            return;
    } catch (Error &err) {
        // something went wrong when files were recreated, set error and rethrow
        onIOError(err.toString());
        return;
    }

    try {
        cman->start();
    } catch (Error &e) {
        onIOError(e.toString());
        throw;
    }

    istats.time_started_ul = istats.time_started_dl = QDateTime::currentDateTime();

    if (prealloc) {
        if (preallocate())
            return;
    }

    continueStart();
}

void TorrentControl::continueStart()
{
    // continues start after the prealloc_thread has finished preallocation
    pman->start(stats.completed && stats.superseeding);
    pman->loadPeerList(tordir + "peer_list"_L1);
    try {
        downloader->loadDownloads(tordir + "current_chunks"_L1);
    } catch (Error &e) {
        // print out warning in case of failure
        // we can still continue the download
        Out(SYS_GEN | LOG_NOTICE) << "Warning : " << e.toString() << endl;
    }

    loadStats();
    stats.running = true;
    stats.started = true;
    stats.queued = false;
    choker_update_timer.update();
    stats_save_timer.update();
    wanted_update_timer.update();

    stalled_timer.update();
    psman->start();
    stalled_timer.update();
    pman->setPartialSeed(!cman->haveAllChunks() && cman->chunksLeft() == 0);
}

void TorrentControl::updateRunningTimes()
{
    QDateTime now = QDateTime::currentDateTime();
    if (!stats.completed)
        istats.running_time_dl += istats.time_started_dl.secsTo(now);
    istats.running_time_ul += istats.time_started_ul.secsTo(now);
    istats.time_started_ul = istats.time_started_dl = now;
}

void TorrentControl::stop(WaitJob *wjob)
{
    if (!stats.paused)
        updateRunningTimes();

    // stop preallocation
    if (job_queue->currentJob() && job_queue->currentJob()->torrentStatus() == ALLOCATING_DISKSPACE)
        job_queue->currentJob()->kill(false);

    if (stats.running) {
        psman->stop(wjob);

        if (tmon)
            tmon->stopped();

        try {
            downloader->saveDownloads(tordir + "current_chunks"_L1);
        } catch (Error &e) {
            // print out warning in case of failure
            // it doesn't corrupt the data, so just a couple of lost chunks
            Out(SYS_GEN | LOG_NOTICE) << "Warning : " << e.toString() << endl;
        }

        downloader->clearDownloads();
    }

    pman->savePeerList(tordir + "peer_list"_L1);
    pman->stop();
    cman->stop();

    stats.running = false;
    stats.autostart = wjob != nullptr;
    stats.queued = false;
    stats.paused = false;
    saveStats();
    updateStatus();
    updateStats();

    Q_EMIT torrentStopped(this);
}

void TorrentControl::setMonitor(MonitorInterface *tmo)
{
    tmon = tmo;
    downloader->setMonitor(tmon);
    if (tmon) {
        const QList<Peer *> ppl = pman->getPeers();
        for (Peer *peer : ppl)
            tmon->peerAdded(peer);
    }

    tor->setMonitor(tmon);
}

void TorrentControl::init(QueueManagerInterface *qman, const QByteArray &data, const QString &tmpdir, const QString &ddir)
{
    m_qman = qman;

    // first load the torrent file
    tor = std::make_unique<Torrent>();
    try {
        tor->load(data, false);
    } catch (bt::Error &err) {
        Out(SYS_GEN | LOG_NOTICE) << "Failed to load torrent: " << err.toString() << endl;
        tor.reset();
        throw Error(i18n("An error occurred while loading <b>%1</b>:<br/><b>%2</b>", loadUrl().toString(), err.toString()));
    }

    tor->setFilePriorityListener(this);
    initInternal(qman, tmpdir, ddir);

    // copy data into torrent file
    QString tor_copy = tordir + "torrent"_L1;
    QFile fptr(tor_copy);
    if (!fptr.open(QIODevice::WriteOnly))
        throw Error(i18n("Unable to create %1: %2", tor_copy, fptr.errorString()));

    fptr.write(data.data(), data.size());
}

void TorrentControl::checkExisting(QueueManagerInterface *qman)
{
    // check if we haven't already loaded the torrent
    // only do this when qman isn't 0
    if (qman && qman->alreadyLoaded(tor->getInfoHash())) {
        if (!stats.priv_torrent) {
            qman->mergeAnnounceList(tor->getInfoHash(), tor->getTrackerList());
            throw Warning(
                i18n("You are already downloading the torrent <b>%1</b>. "
                     "The tracker lists from both torrents have been merged.",
                     tor->getNameSuggestion()));
        } else {
            throw Warning(i18n("You are already downloading the torrent <b>%1</b>.", tor->getNameSuggestion()));
        }
    }
}

void TorrentControl::setupDirs(const QString &tmpdir, const QString &ddir)
{
    tordir = tmpdir;

    if (!tordir.endsWith(DirSeparator()))
        tordir += DirSeparator();

    outputdir = ddir.trimmed();
    if (outputdir.length() > 0 && !outputdir.endsWith(DirSeparator()))
        outputdir += DirSeparator();

    if (!bt::Exists(tordir)) {
        bt::MakeDir(tordir);
    }
}

void TorrentControl::setupStats()
{
    stats.completed = false;
    stats.running = false;
    stats.torrent_name = tor->getNameSuggestion();
    stats.multi_file_torrent = tor->isMultiFile();
    stats.total_bytes = tor->getTotalSize();
    stats.priv_torrent = tor->isPrivate();

    // check the stats file for the custom_output_name variable
    if (!stats_file)
        stats_file = std::make_unique<StatsFile>(tordir + "stats"_L1);

    if (stats_file->hasKey(u"CUSTOM_OUTPUT_NAME"_s) && stats_file->readULong(u"CUSTOM_OUTPUT_NAME"_s) == 1) {
        istats.custom_output_name = true;
    }

    if (stats.time_added.isNull())
        stats.time_added = QDateTime::currentDateTime();

    // load outputdir if outputdir is null
    if (outputdir.isNull() || outputdir.length() == 0)
        loadOutputDir();
}

void TorrentControl::setupData()
{
    // create PeerManager and Tracker
    pman = std::make_unique<PeerManager>(*tor);
    // Out() << "Tracker url " << url << " " << url.protocol() << " " << url.prettyURL() << endl;
    psman = std::make_unique<PeerSourceManager>(this, pman.get());

    // Create chunkmanager, load the index file if it exists
    // else create all the necesarry files
    cman = std::make_unique<ChunkManager>(*tor, tordir, outputdir, istats.custom_output_name, cache_factory.get());
    if (bt::Exists(tordir + "index"_L1))
        cman->loadIndexFile();

    connect(cman.get(), &ChunkManager::updateStats, this, &TorrentControl::updateStats);
    updateStats();
    stats.completed = cman->completed();

    // create downloader, uploader and choker
    downloader = std::make_unique<Downloader>(*tor, *pman, *cman);
    downloader->loadWebSeeds(tordir + "webseeds"_L1);
    connect(downloader.get(), &Downloader::ioError, this, &TorrentControl::onIOError);
    connect(downloader.get(), &Downloader::chunkDownloaded, this, &TorrentControl::downloaded);
    uploader = std::make_unique<Uploader>(*cman, *pman);
    choke = std::make_unique<Choker>(*pman, *cman);

    connect(pman.get(), &PeerManager::newPeer, this, &TorrentControl::onNewPeer);
    connect(pman.get(), &PeerManager::peerKilled, this, &TorrentControl::onPeerRemoved);
    connect(cman.get(), &ChunkManager::excluded, downloader.get(), &Downloader::onExcluded);
    connect(cman.get(), &ChunkManager::included, downloader.get(), &Downloader::onIncluded);
    connect(cman.get(), &ChunkManager::corrupted, this, &TorrentControl::corrupted);
}

void TorrentControl::initInternal(QueueManagerInterface *qman, const QString &tmpdir, const QString &ddir)
{
    checkExisting(qman);
    setupDirs(tmpdir, ddir);
    setupStats();
    setupData();
    updateStatus();

    // to get rid of phantom bytes we need to take into account
    // the data from downloads already in progress
    try {
        Uint64 db = downloader->bytesDownloaded();
        Uint64 cb = downloader->getDownloadedBytesOfCurrentChunksFile(tordir + "current_chunks"_L1);
        istats.prev_bytes_dl = db + cb;

        //  Out() << "Downloaded : " << BytesToString(db) << endl;
        //  Out() << "current_chunks : " << BytesToString(cb) << endl;
    } catch (Error &e) {
        // print out warning in case of failure
        Out(SYS_GEN | LOG_DEBUG) << "Warning : " << e.toString() << endl;
        istats.prev_bytes_dl = downloader->bytesDownloaded();
    }

    loadStats();
    updateStats();
    saveStats();
    stats.output_path = cman->getOutputPath();
    updateStatus();
}

void TorrentControl::setDisplayName(const QString &n)
{
    display_name = n;
    saveStats();
}

bool TorrentControl::announceAllowed()
{
    return psman != nullptr && stats.running;
}

void TorrentControl::updateTracker()
{
    if (announceAllowed()) {
        psman->manualUpdate();
    }
}

void TorrentControl::scrapeTracker()
{
    psman->scrape();
}

void TorrentControl::onNewPeer(Peer *p)
{
    if (!stats.superseeding) {
        // Only send which chunks we have when we are not superseeding
        if (p->getStats().fast_extensions) {
            const BitSet &bs = cman->getBitSet();
            if (bs.allOn())
                p->sendHaveAll();
            else if (bs.numOnBits() == 0)
                p->sendHaveNone();
            else
                p->sendBitSet(bs);
        } else {
            p->sendBitSet(cman->getBitSet());
        }
    }

    if (!stats.completed && !stats.paused)
        p->sendInterested();

    if (!stats.priv_torrent) {
        if (p->isDHTSupported())
            p->sendPort(Globals::instance().getDHT().getPort());
        else
            // WORKAROUND so we can contact µTorrent's DHT
            // They do not properly support the standard and do not turn on
            // the DHT bit in the handshake, so we just ping each peer by default.
            p->emitPortPacket();
    }

    // set group ID's for traffic shaping
    p->setGroupIDs(upload_gid, download_gid);
    downloader->addPieceDownloader(p->getPeerDownloader());
    if (tmon)
        tmon->peerAdded(p);
}

void TorrentControl::onPeerRemoved(Peer *p)
{
    downloader->removePieceDownloader(p->getPeerDownloader());
    if (tmon)
        tmon->peerRemoved(p);
}

void TorrentControl::doChoking()
{
    choke->update(stats.completed, stats);
}

bool TorrentControl::changeTorDir(const QString &new_dir)
{
    int pos = tordir.lastIndexOf(bt::DirSeparator(), -2);
    if (pos == -1) {
        Out(SYS_GEN | LOG_DEBUG) << "Could not find torX part in " << tordir << endl;
        return false;
    }

    QString ntordir = new_dir + tordir.mid(pos + 1);

    Out(SYS_GEN | LOG_DEBUG) << tordir << " -> " << ntordir << endl;
    try {
        bt::Move(tordir, ntordir);
        old_tordir = tordir;
        tordir = ntordir;
    } catch (Error &err) {
        Out(SYS_GEN | LOG_IMPORTANT) << "Could not move " << tordir << " to " << ntordir << endl;
        return false;
    }

    cman->changeDataDir(tordir);
    return true;
}

bool TorrentControl::changeOutputDir(const QString &ndir, int flags)
{
    // check if torrent is running and stop it before moving data
    QString new_dir = ndir;
    if (!new_dir.endsWith(bt::DirSeparator()))
        new_dir += bt::DirSeparator();

    try {
        QString nd;
        if (!(flags & bt::TorrentInterface::FULL_PATH)) {
            if (istats.custom_output_name) {
                int slash_pos = stats.output_path.lastIndexOf(bt::DirSeparator(), -2);
                nd = new_dir + stats.output_path.mid(slash_pos + 1);
            } else {
                nd = new_dir + tor->getNameSuggestion();
            }
        } else {
            nd = new_dir;
        }

        if (stats.output_path != nd) {
            move_data_files_destination_path = nd;
            Job *j = nullptr;
            if (flags & bt::TorrentInterface::MOVE_FILES) {
                if (stats.multi_file_torrent)
                    j = cman->moveDataFiles(nd);
                else
                    j = cman->moveDataFiles(new_dir);
            }

            if (j) {
                j->setTorrent(this);
                connect(j, &Job::result, this, &TorrentControl::moveDataFilesFinished);
                job_queue->enqueue(j);
                return true;
            } else {
                moveDataFilesFinished(nullptr);
            }
        } else {
            Out(SYS_GEN | LOG_NOTICE) << "Source is the same as destination, so doing nothing" << endl;
        }
    } catch (Error &err) {
        Out(SYS_GEN | LOG_IMPORTANT) << "Could not move " << stats.output_path << " to " << new_dir << ". Exception: " << endl;
        return false;
    }

    return true;
}

void TorrentControl::moveDataFilesFinished(KJob *kj)
{
    const auto job = static_cast<bt::Job *>(kj);
    if (job)
        cman->moveDataFilesFinished(job);

    if (!job || (job && !job->error())) {
        cman->changeOutputPath(move_data_files_destination_path);
        outputdir = stats.output_path = move_data_files_destination_path;
        istats.custom_output_name = true;

        saveStats();
        Out(SYS_GEN | LOG_NOTICE) << "Data directory changed for torrent "
                                  << "'" << stats.torrent_name << "' to: " << move_data_files_destination_path << endl;
    } else if (job->error()) {
        Out(SYS_GEN | LOG_IMPORTANT) << "Could not move " << stats.output_path << " to " << move_data_files_destination_path << endl;
    }
}

bool TorrentControl::moveTorrentFiles(const QMap<TorrentFileInterface *, QString> &files)
{
    try {
        Job *j = cman->moveDataFiles(files);
        if (j) {
            connect(j, &Job::result, this, &TorrentControl::moveDataFilesWithMapFinished);
            job_queue->enqueue(j);
        }

    } catch (Error &err) {
        return false;
    }

    return true;
}

void TorrentControl::moveDataFilesWithMapFinished(KJob *j)
{
    if (!j)
        return;

    const auto job = static_cast<MoveDataFilesJob *>(j);
    cman->moveDataFilesFinished(job->fileMap(), job);
    Out(SYS_GEN | LOG_NOTICE) << "Move of data files completed " << endl;
}

void TorrentControl::rollback()
{
    try {
        bt::Move(tordir, old_tordir);
        tordir = old_tordir;
        cman->changeDataDir(tordir);
    } catch (Error &err) {
        Out(SYS_GEN | LOG_IMPORTANT) << "Could not move " << tordir << " to " << old_tordir << endl;
    }
}

void TorrentControl::updateStatus()
{
    TorrentStatus old = stats.status;
    if (stats.stopped_by_error)
        stats.status = ERROR;
    else if (job_queue->currentJob() && job_queue->currentJob()->torrentStatus() != INVALID_STATUS)
        stats.status = job_queue->currentJob()->torrentStatus();
    else if (stats.queued)
        stats.status = QUEUED;
    else if (stats.completed && (overMaxRatio() || overMaxSeedTime()))
        stats.status = SEEDING_COMPLETE;
    else if (!stats.running && stats.completed)
        stats.status = DOWNLOAD_COMPLETE;
    else if (!stats.started)
        stats.status = NOT_STARTED;
    else if (!stats.running)
        stats.status = STOPPED;
    else if (stats.running && stats.paused)
        stats.status = PAUSED;
    else if (stats.running && stats.completed)
        stats.status = stats.superseeding ? SUPERSEEDING : SEEDING;
    else if (stats.running)
        // protocol messages are also included in speed calculation, so lets not compare with 0
        stats.status = downloader->downloadRate() > 100 ? DOWNLOADING : STALLED;

    if (old != stats.status)
        Q_EMIT statusChanged(this);
}

const BitSet &TorrentControl::downloadedChunksBitSet() const
{
    if (cman)
        return cman->getBitSet();
    else
        return BitSet::null;
}

const BitSet &TorrentControl::availableChunksBitSet() const
{
    if (!pman)
        return BitSet::null;
    else
        return pman->getAvailableChunksBitSet();
}

const BitSet &TorrentControl::excludedChunksBitSet() const
{
    if (!cman)
        return BitSet::null;
    else
        return cman->getExcludedBitSet();
}

const BitSet &TorrentControl::onlySeedChunksBitSet() const
{
    if (!cman)
        return BitSet::null;
    else
        return cman->getOnlySeedBitSet();
}

void TorrentControl::saveStats()
{
    if (loading_stats) // don't save while we are loading
        return;

    if (!stats_file)
        stats_file = std::make_unique<StatsFile>(tordir + "stats"_L1);

    stats_file->write(u"OUTPUTDIR"_s, cman->getDataDir());
    stats_file->write(u"COMPLETEDDIR"_s, completed_dir);

    if (cman->getDataDir() != outputdir)
        outputdir = cman->getDataDir();

    stats_file->write(u"UPLOADED"_s, QString::number(uploader->bytesUploaded()));

    if (stats.running) {
        QDateTime now = QDateTime::currentDateTime();
        if (!stats.completed)
            stats_file->write(u"RUNNING_TIME_DL"_s, QString::number(istats.running_time_dl + istats.time_started_dl.secsTo(now)));
        else
            stats_file->write(u"RUNNING_TIME_DL"_s, QString::number(istats.running_time_dl));
        stats_file->write(u"RUNNING_TIME_UL"_s, QString::number(istats.running_time_ul + istats.time_started_ul.secsTo(now)));
    } else {
        stats_file->write(u"RUNNING_TIME_DL"_s, QString::number(istats.running_time_dl));
        stats_file->write(u"RUNNING_TIME_UL"_s, QString::number(istats.running_time_ul));
    }

    stats_file->write(u"QM_CAN_START"_s, stats.qm_can_start ? u"1"_s : u"0"_s);
    stats_file->write(u"PRIORITY"_s, QString::number(istats.priority));
    stats_file->write(u"AUTOSTART"_s, QString::number(stats.autostart));
    stats_file->write(u"IMPORTED"_s, QString::number(stats.imported_bytes));
    stats_file->write(u"CUSTOM_OUTPUT_NAME"_s, istats.custom_output_name ? u"1"_s : u"0"_s);
    stats_file->write(u"MAX_RATIO"_s, u"%1"_s.arg(stats.max_share_ratio, 0, 'f', 2));
    stats_file->write(u"MAX_SEED_TIME"_s, QString::number(stats.max_seed_time));
    stats_file->write(u"RESTART_DISK_PREALLOCATION"_s, prealloc ? u"1"_s : u"0"_s);
    stats_file->write(u"AUTO_STOPPED"_s, stats.auto_stopped ? u"1"_s : u"0"_s);
    stats_file->write(u"LAST_DOWNLOAD_ACTIVITY"_s, QString::number(stats.last_download_activity_time));
    stats_file->write(u"LAST_UPLOAD_ACTIVITY"_s, QString::number(stats.last_upload_activity_time));

    if (!stats.priv_torrent) {
        // save dht and pex
        stats_file->write(u"DHT"_s, isFeatureEnabled(DHT_FEATURE) ? u"1"_s : u"0"_s);
        stats_file->write(u"UT_PEX"_s, isFeatureEnabled(UT_PEX_FEATURE) ? u"1"_s : u"0"_s);
    }

    stats_file->write(u"UPLOAD_LIMIT"_s, QString::number(upload_limit));
    stats_file->write(u"DOWNLOAD_LIMIT"_s, QString::number(download_limit));
    stats_file->write(u"ASSURED_UPLOAD_SPEED"_s, QString::number(assured_upload_speed));
    stats_file->write(u"ASSURED_DOWNLOAD_SPEED"_s, QString::number(assured_download_speed));
    if (!user_modified_name.isEmpty())
        stats_file->write(u"USER_MODIFIED_NAME"_s, user_modified_name);
    stats_file->write(u"DISPLAY_NAME"_s, display_name);
    stats_file->write(u"URL"_s, url.toDisplayString());

    stats_file->write(u"TIME_ADDED"_s, QString::number(stats.time_added.toSecsSinceEpoch()));
    stats_file->write(u"SUPERSEEDING"_s, stats.superseeding ? u"1"_s : u"0"_s);

    stats_file->sync();
}

void TorrentControl::loadStats()
{
    if (!bt::Exists(tordir + "stats"_L1)) {
        setFeatureEnabled(DHT_FEATURE, true);
        setFeatureEnabled(UT_PEX_FEATURE, true);
        return;
    }

    RecursiveEntryGuard guard(&loading_stats);
    if (!stats_file)
        stats_file = std::make_unique<StatsFile>(tordir + "stats"_L1);

    Uint64 val = stats_file->readUint64(u"UPLOADED"_s);
    // stats.session_bytes_uploaded will be calculated based upon prev_bytes_ul
    // seeing that this will change here, we need to save it
    istats.session_bytes_uploaded = stats.session_bytes_uploaded;
    istats.prev_bytes_ul = val;
    uploader->setBytesUploaded(val);

    istats.running_time_dl = stats_file->readULong(u"RUNNING_TIME_DL"_s);
    istats.running_time_ul = stats_file->readULong(u"RUNNING_TIME_UL"_s);
    // make sure runtime ul is always equal or largen then running time dl
    // in case something got corrupted
    if (istats.running_time_ul < istats.running_time_dl)
        istats.running_time_ul = istats.running_time_dl;

    outputdir = stats_file->readString(u"OUTPUTDIR"_s).trimmed();
    if (stats_file->hasKey(u"CUSTOM_OUTPUT_NAME"_s) && stats_file->readULong(u"CUSTOM_OUTPUT_NAME"_s) == 1) {
        istats.custom_output_name = true;
    }

    if (stats_file->hasKey(u"COMPLETEDDIR"_s)) {
        completed_dir = stats_file->readString(u"COMPLETEDDIR"_s);
        if (completed_dir == outputdir)
            completed_dir = QString();
    }

    if (stats_file->hasKey(u"USER_MODIFIED_NAME"_s))
        user_modified_name = stats_file->readString(u"USER_MODIFIED_NAME"_s);

    if (stats_file->hasKey(u"DISPLAY_NAME"_s))
        display_name = stats_file->readString(u"DISPLAY_NAME"_s);

    istats.priority = stats_file->readInt(u"PRIORITY"_s);
    stats.autostart = stats_file->readBoolean(u"AUTOSTART"_s);
    stats.imported_bytes = stats_file->readUint64(u"IMPORTED"_s);
    stats.max_share_ratio = stats_file->readFloat(u"MAX_RATIO"_s);
    stats.max_seed_time = stats_file->readFloat(u"MAX_SEED_TIME"_s);
    stats.qm_can_start = stats_file->readBoolean(u"QM_CAN_START"_s);
    stats.auto_stopped = stats_file->readBoolean(u"AUTO_STOPPED"_s);

    if (stats_file->hasKey(u"RESTART_DISK_PREALLOCATION"_s))
        prealloc = stats_file->readString(u"RESTART_DISK_PREALLOCATION"_s) == '1'_L1;

    if (!stats.priv_torrent) {
        if (stats_file->hasKey(u"DHT"_s))
            istats.dht_on = stats_file->readBoolean(u"DHT"_s);
        else
            istats.dht_on = true;

        setFeatureEnabled(DHT_FEATURE, istats.dht_on);
        if (stats_file->hasKey(u"UT_PEX"_s))
            setFeatureEnabled(UT_PEX_FEATURE, stats_file->readBoolean(u"UT_PEX"_s));
    }

    Uint32 aup = stats_file->readInt(u"ASSURED_UPLOAD_SPEED"_s);
    Uint32 adown = stats_file->readInt(u"ASSURED_DOWNLOAD_SPEED"_s);
    Uint32 up = stats_file->readInt(u"UPLOAD_LIMIT"_s);
    Uint32 down = stats_file->readInt(u"DOWNLOAD_LIMIT"_s);
    setDownloadProps(down, adown);
    setUploadProps(up, aup);
    pman->setGroupIDs(upload_gid, download_gid);

    url = QUrl(stats_file->readString(u"URL"_s));

    if (stats_file->hasKey(u"TIME_ADDED"_s))
        stats.time_added.setSecsSinceEpoch(stats_file->readULong(u"TIME_ADDED"_s));
    else
        stats.time_added = QDateTime::currentDateTime();

    if (stats_file->hasKey(u"LAST_DOWNLOAD_ACTIVITY"_s)) {
        stats.last_download_activity_time = stats_file->readULong(u"LAST_DOWNLOAD_ACTIVITY"_s);
    } else {
        stats.last_download_activity_time = QDateTime::currentMSecsSinceEpoch();
    }

    if (stats_file->hasKey(u"LAST_UPLOAD_ACTIVITY"_s)) {
        stats.last_upload_activity_time = stats_file->readULong(u"LAST_UPLOAD_ACTIVITY"_s);
    } else {
        stats.last_upload_activity_time = QDateTime::currentMSecsSinceEpoch();
    }

    bool ss = stats_file->hasKey(u"SUPERSEEDING"_s) && stats_file->readBoolean(u"SUPERSEEDING"_s);
    setSuperSeeding(ss);
}

void TorrentControl::loadOutputDir()
{
    if (!stats_file)
        stats_file = std::make_unique<StatsFile>(tordir + "stats"_L1);

    if (!stats_file->hasKey(u"OUTPUTDIR"_s))
        return;

    outputdir = stats_file->readString(u"OUTPUTDIR"_s).trimmed();
    if (stats_file->hasKey(u"CUSTOM_OUTPUT_NAME"_s) && stats_file->readULong(u"CUSTOM_OUTPUT_NAME"_s) == 1) {
        istats.custom_output_name = true;
    }
}

bool TorrentControl::readyForPreview() const
{
    if (tor->isMultiFile() || !tor->isMultimedia())
        return false;

    Uint32 preview_range = cman->previewChunkRangeSize();
    if (preview_range == 0)
        return false;

    const BitSet &bs = downloadedChunksBitSet();
    for (Uint32 i = 0; i < qMin(preview_range, bs.getNumBits()); i++) {
        if (!bs.get(i))
            return false;
    }
    return true;
}

bool TorrentControl::isMultimedia() const
{
    return !tor->isMultiFile() && tor->isMultimedia();
}

void TorrentControl::updateStats()
{
    stats.num_chunks_downloading = downloader ? downloader->numActiveDownloads() : 0;
    stats.num_peers = pman ? pman->getNumConnectedPeers() : 0;
    stats.upload_rate = uploader && stats.running ? uploader->uploadRate() : 0;
    stats.download_rate = downloader && stats.running ? downloader->downloadRate() : 0;
    stats.bytes_left = cman ? cman->bytesLeft() : 0;
    stats.bytes_left_to_download = cman ? cman->bytesLeftToDownload() : 0;
    stats.bytes_uploaded = uploader ? uploader->bytesUploaded() : 0;
    stats.bytes_downloaded = downloader ? downloader->bytesDownloaded() : 0;
    stats.total_chunks = tor ? tor->getNumChunks() : 0;
    stats.num_chunks_downloaded = cman ? cman->chunksDownloaded() : 0;
    stats.num_chunks_excluded = cman ? cman->chunksExcluded() : 0;
    stats.chunk_size = tor ? tor->getChunkSize() : 0;
    stats.num_chunks_left = cman ? cman->chunksLeft() : 0;
    stats.total_bytes_to_download = (tor && cman) ? tor->getTotalSize() - cman->bytesExcluded() : 0;

    if (stats.bytes_downloaded >= istats.prev_bytes_dl)
        stats.session_bytes_downloaded = stats.bytes_downloaded - istats.prev_bytes_dl;
    else
        stats.session_bytes_downloaded = 0;

    if (stats.bytes_uploaded >= istats.prev_bytes_ul)
        stats.session_bytes_uploaded = (stats.bytes_uploaded - istats.prev_bytes_ul) + istats.session_bytes_uploaded;
    else
        stats.session_bytes_uploaded = istats.session_bytes_uploaded;

    getSeederInfo(stats.seeders_total, stats.seeders_connected_to);
    getLeecherInfo(stats.leechers_total, stats.leechers_connected_to);
}

void TorrentControl::trackerScrapeDone()
{
    stats.seeders_total = psman->getNumSeeders();
    stats.leechers_total = psman->getNumLeechers();
}

void TorrentControl::afterRename()
{
    stats_file->write(u"OUTPUTDIR"_s, cman->getDataDir());
    stats_file->write(u"USER_MODIFIED_NAME"_s, user_modified_name);
    stats_file->sync();
    cman->saveFileMap();
}

void TorrentControl::getSeederInfo(Uint32 &total, Uint32 &connected_to) const
{
    total = 0;
    connected_to = 0;
    if (!pman || !psman)
        return;

    connected_to = pman->getNumConnectedSeeders();
    total = psman->getNumSeeders();
    if (total == 0)
        total = connected_to;
}

void TorrentControl::getLeecherInfo(Uint32 &total, Uint32 &connected_to) const
{
    total = 0;
    connected_to = 0;
    if (!pman || !psman)
        return;

    connected_to = pman->getNumConnectedLeechers();
    total = psman->getNumLeechers();
    if (total == 0)
        total = connected_to;
}

Uint32 TorrentControl::getRunningTimeDL() const
{
    if (!stats.running || stats.completed || stats.paused)
        return istats.running_time_dl;
    else
        return istats.running_time_dl + istats.time_started_dl.secsTo(QDateTime::currentDateTime());
}

Uint32 TorrentControl::getRunningTimeUL() const
{
    if (!stats.running || stats.paused)
        return istats.running_time_ul;
    else
        return istats.running_time_ul + istats.time_started_ul.secsTo(QDateTime::currentDateTime());
}

Uint32 TorrentControl::getNumFiles() const
{
    if (tor && tor->isMultiFile())
        return tor->getNumFiles();
    else
        return 0;
}

TorrentFileInterface &TorrentControl::getTorrentFile(Uint32 index)
{
    if (tor)
        return tor->getFile(index);
    else
        return TorrentFile::null;
}

const TorrentFileInterface &TorrentControl::getTorrentFile(Uint32 index) const
{
    if (tor)
        return tor->getFile(index);
    else
        return TorrentFile::null;
}

void TorrentControl::setPriority(int p)
{
    istats.priority = p;
    if (!stats_file)
        stats_file = std::make_unique<StatsFile>(tordir + "stats"_L1);

    stats_file->write(u"PRIORITY"_s, QString::number(istats.priority));
    updateStatus();
}

void TorrentControl::setMaxShareRatio(float ratio)
{
    if (ratio == 1.00f) {
        if (stats.max_share_ratio != ratio)
            stats.max_share_ratio = ratio;
    } else
        stats.max_share_ratio = ratio;

    saveStats();
    Q_EMIT maxRatioChanged(this);
}

void TorrentControl::setMaxSeedTime(float hours)
{
    stats.max_seed_time = hours;
    saveStats();
}

bool TorrentControl::overMaxRatio()
{
    return stats.overMaxRatio();
}

bool TorrentControl::overMaxSeedTime()
{
    if (stats.completed && stats.max_seed_time > 0) {
        Uint32 dl = getRunningTimeDL();
        Uint32 ul = getRunningTimeUL();
        if ((ul - dl) / 3600.0f > stats.max_seed_time)
            return true;
    }

    return false;
}

TrackersList *TorrentControl::getTrackersList()
{
    return psman.get();
}

const TrackersList *TorrentControl::getTrackersList() const
{
    return psman.get();
}

Job *TorrentControl::startDataCheck(bool auto_import, bt::Uint32 from, bt::Uint32 to)
{
    Job *j = new DataCheckerJob(auto_import, this, from, to);
    job_queue->enqueue(j);
    return j;
}

void TorrentControl::beforeDataCheck()
{
    stats.status = CHECKING_DATA;
    stats.num_corrupted_chunks = 0; // reset the number of corrupted chunks found
    Q_EMIT statusChanged(this);
}

void TorrentControl::afterDataCheck(DataCheckerJob *job, const BitSet &result)
{
    bool completed = stats.completed;
    if (job && !job->isStopped()) {
        downloader->dataChecked(result, job->firstChunk(), job->lastChunk());
        // update chunk manager
        cman->dataChecked(result, job->firstChunk(), job->lastChunk());
        if (job->isAutoImport()) {
            downloader->recalcDownloaded();
            stats.imported_bytes = downloader->bytesDownloaded();
            stats.completed = cman->completed();
        } else {
            Uint64 downloaded = stats.bytes_downloaded;
            downloader->recalcDownloaded();
            updateStats();
            if (stats.bytes_downloaded > downloaded)
                stats.imported_bytes = stats.bytes_downloaded - downloaded;

            stats.completed = cman->completed();
            pman->setPartialSeed(!cman->haveAllChunks() && cman->chunksLeft() == 0);
        }
    }

    saveStats();
    updateStats();
    Out(SYS_GEN | LOG_NOTICE) << "Data check finished" << endl;
    updateStatus();
    Q_EMIT dataCheckFinished();

    if (stats.completed != completed) {
        // Tell QM to redo queue and emit finished signal
        // seeder might have become downloader, so
        // queue might need to be redone
        // use QTimer because we need to ensure this is run after the JobQueue removes the job
        QTimer::singleShot(0, this, SIGNAL(updateQueue()));
        if (stats.completed)
            QTimer::singleShot(0, this, &TorrentControl::emitFinished);
    }
}

void TorrentControl::emitFinished()
{
    Q_EMIT finished(this);
}

void TorrentControl::markExistingFilesAsDownloaded()
{
    cman->markExistingFilesAsDownloaded();
    downloader->recalcDownloaded();
    stats.imported_bytes = downloader->bytesDownloaded();
    if (cman->haveAllChunks())
        stats.completed = true;

    updateStats();
}

bool TorrentControl::hasExistingFiles() const
{
    return cman->hasExistingFiles();
}

bool TorrentControl::isStorageMounted(QStringList &missing)
{
    return cman->isStorageMounted(missing);
}

bool TorrentControl::hasMissingFiles(QStringList &sl)
{
    return cman->hasMissingFiles(sl);
}

void TorrentControl::recreateMissingFiles()
{
    try {
        cman->recreateMissingFiles();
        prealloc = true; // set prealloc to true so files will be truncated again
        downloader->dataChecked(cman->getBitSet(), 0, tor->getNumChunks() - 1); // update chunk selector
    } catch (Error &err) {
        onIOError(err.toString());
        throw;
    }
}

void TorrentControl::dndMissingFiles()
{
    try {
        cman->dndMissingFiles();
        prealloc = true; // set prealloc to true so files will be truncated again
        Q_EMIT missingFilesMarkedDND(this);
        downloader->dataChecked(cman->getBitSet(), 0, tor->getNumChunks() - 1); // update chunk selector
    } catch (Error &err) {
        onIOError(err.toString());
        throw;
    }
}

void TorrentControl::handleError(const QString &err)
{
    onIOError(err);
}

Uint32 TorrentControl::getNumDHTNodes() const
{
    return tor->getNumDHTNodes();
}

const DHTNode &TorrentControl::getDHTNode(Uint32 i) const
{
    return tor->getDHTNode(i);
}

void TorrentControl::deleteDataFiles()
{
    if (!cman)
        return;

    Job *job = cman->deleteDataFiles();
    if (job)
        job->start(); // don't use queue, this is only done when removing the torrent
}

const bt::SHA1Hash &TorrentControl::getInfoHash() const
{
    return tor->getInfoHash();
}

void TorrentControl::addPeerSource(PeerSource *ps)
{
    if (psman)
        psman->addPeerSource(ps);
}

void TorrentControl::removePeerSource(PeerSource *ps)
{
    if (psman)
        psman->removePeerSource(ps);
}

void TorrentControl::corrupted(Uint32 chunk)
{
    // make sure we will redownload the chunk
    downloader->corrupted(chunk);
    if (stats.completed)
        stats.completed = false;

    // emit signal to show a systray message
    stats.num_corrupted_chunks++;
    Q_EMIT corruptedDataFound(this);
}

int TorrentControl::getETA()
{
    return m_eta->estimate();
}

const bt::PeerID &TorrentControl::getOwnPeerID() const
{
    return tor->getPeerID();
}

bool TorrentControl::isFeatureEnabled(TorrentFeature tf)
{
    switch (tf) {
    case DHT_FEATURE:
        return psman->dhtStarted();
    case UT_PEX_FEATURE:
        return pman->isPexEnabled();
    default:
        return false;
    }
}

void TorrentControl::setFeatureEnabled(TorrentFeature tf, bool on)
{
    switch (tf) {
    case DHT_FEATURE:
        if (on) {
            if (!stats.priv_torrent) {
                psman->addDHT();
                istats.dht_on = psman->dhtStarted();
                saveStats();
            }
        } else {
            psman->removeDHT();
            istats.dht_on = false;
            saveStats();
        }
        break;
    case UT_PEX_FEATURE:
        if (on) {
            if (!stats.priv_torrent && !pman->isPexEnabled()) {
                pman->setPexEnabled(true);
            }
        } else {
            pman->setPexEnabled(false);
        }
        break;
    }
}

void TorrentControl::createFiles()
{
    cman->createFiles(true);
    stats.output_path = cman->getOutputPath();
}

bool TorrentControl::checkDiskSpace(bool emit_sig)
{
    last_diskspace_check = bt::CurrentTime();

    // calculate free disk space
    Uint64 bytes_free = 0;
    if (FreeDiskSpace(getDataDir(), bytes_free)) {
        Out(SYS_GEN | LOG_DEBUG) << "FreeBytes " << BytesToString(bytes_free) << endl;
        Uint64 bytes_to_download = stats.total_bytes_to_download;
        Uint64 downloaded = 0;
        try {
            downloaded = cman->diskUsage();
            Out(SYS_GEN | LOG_DEBUG) << "Downloaded " << BytesToString(downloaded) << endl;
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Error : " << err.toString() << endl;
        }
        Uint64 remaining = 0;
        if (downloaded <= bytes_to_download)
            remaining = bytes_to_download - downloaded;

        Out(SYS_GEN | LOG_DEBUG) << "Remaining " << BytesToString(remaining) << endl;
        if (remaining > bytes_free) {
            bool toStop = bytes_free < (Uint64)min_diskspace * 1024 * 1024;

            // if we don't need to stop the torrent, only emit the signal once
            // so that we do bother the user continuously
            if (emit_sig && (toStop || !istats.diskspace_warning_emitted)) {
                Q_EMIT diskSpaceLow(this, toStop);
                istats.diskspace_warning_emitted = true;
            }

            if (!stats.running) {
                stats.status = NO_SPACE_LEFT;
                Q_EMIT statusChanged(this);
            }

            return false;
        }
    }

    return true;
}

void TorrentControl::setUploadProps(Uint32 limit, Uint32 rate)
{
    net::SocketMonitor &smon = net::SocketMonitor::instance();
    if (upload_gid) {
        if (!limit && !rate) {
            smon.removeGroup(net::SocketMonitor::UPLOAD_GROUP, upload_gid);
            upload_gid = 0;
        } else {
            smon.setGroupLimit(net::SocketMonitor::UPLOAD_GROUP, upload_gid, limit);
            smon.setGroupAssuredRate(net::SocketMonitor::UPLOAD_GROUP, upload_gid, rate);
        }
    } else {
        if (limit || rate) {
            upload_gid = smon.newGroup(net::SocketMonitor::UPLOAD_GROUP, limit, rate);
        }
    }

    upload_limit = limit;
    assured_upload_speed = rate;
}

void TorrentControl::setDownloadProps(Uint32 limit, Uint32 rate)
{
    net::SocketMonitor &smon = net::SocketMonitor::instance();
    if (download_gid) {
        if (!limit && !rate) {
            smon.removeGroup(net::SocketMonitor::DOWNLOAD_GROUP, download_gid);
            download_gid = 0;
        } else {
            smon.setGroupLimit(net::SocketMonitor::DOWNLOAD_GROUP, download_gid, limit);
            smon.setGroupAssuredRate(net::SocketMonitor::DOWNLOAD_GROUP, download_gid, rate);
        }
    } else {
        if (limit || rate) {
            download_gid = smon.newGroup(net::SocketMonitor::DOWNLOAD_GROUP, limit, rate);
        }
    }

    download_limit = limit;
    assured_download_speed = rate;
}

void TorrentControl::setTrafficLimits(Uint32 up, Uint32 down)
{
    setDownloadProps(down, assured_download_speed);
    setUploadProps(up, assured_upload_speed);
    saveStats();
    pman->setGroupIDs(upload_gid, download_gid);
    downloader->setGroupIDs(upload_gid, download_gid);
}

void TorrentControl::getTrafficLimits(Uint32 &up, Uint32 &down)
{
    up = upload_limit;
    down = download_limit;
}

void TorrentControl::setAssuredSpeeds(Uint32 up, Uint32 down)
{
    setDownloadProps(download_limit, down);
    setUploadProps(upload_limit, up);
    saveStats();
    pman->setGroupIDs(upload_gid, download_gid);
    downloader->setGroupIDs(upload_gid, download_gid);
}

void TorrentControl::getAssuredSpeeds(Uint32 &up, Uint32 &down)
{
    up = assured_upload_speed;
    down = assured_download_speed;
}

const PeerManager *TorrentControl::getPeerMgr() const
{
    return pman.get();
}

void TorrentControl::preallocFinished(const QString &error, bool completed)
{
    Out(SYS_GEN | LOG_DEBUG) << "preallocFinished " << error << " " << completed << endl;
    if (!error.isEmpty() || !completed) {
        // upon error just call onIOError and return
        if (!error.isEmpty())
            onIOError(error);
        prealloc = true; // still need to do preallocation
    } else {
        // continue the startup of the torrent
        prealloc = false;
        stats.status = NOT_STARTED;
        saveStats();
        continueStart();
        Q_EMIT statusChanged(this);
    }
}

void TorrentControl::setCacheFactory(std::unique_ptr<CacheFactory> cf)
{
    cache_factory = std::move(cf);
}

Uint32 TorrentControl::getNumWebSeeds() const
{
    return downloader->getNumWebSeeds();
}

const WebSeedInterface *TorrentControl::getWebSeed(Uint32 i) const
{
    return downloader->getWebSeed(i);
}

WebSeedInterface *TorrentControl::getWebSeed(Uint32 i)
{
    return downloader->getWebSeed(i);
}

bool TorrentControl::addWebSeed(const QUrl &url)
{
    WebSeed *ws = downloader->addWebSeed(url);
    if (ws) {
        downloader->saveWebSeeds(tordir + "webseeds"_L1);
        ws->setGroupIDs(upload_gid, download_gid); // make sure webseed has proper group ID
    }
    return ws != nullptr;
}

bool TorrentControl::removeWebSeed(const QUrl &url)
{
    bool ret = downloader->removeWebSeed(url);
    if (ret)
        downloader->saveWebSeeds(tordir + "webseeds"_L1);
    return ret;
}

void TorrentControl::setUserModifiedFileName(const QString &n)
{
    TorrentInterface::setUserModifiedFileName(n);
    QString path = stats.output_path;
    if (path.endsWith(bt::DirSeparator())) {
        // remove trailing dir separator or QFileInfo::absolutePath used below
        // will only remove the trailing separator
        path.removeLast();
    }
    QFileInfo fi(path);
    path = fi.absolutePath();
    if (!path.endsWith(bt::DirSeparator())) {
        // re-add the trailing dir separator
        path += bt::DirSeparator();
    }

    cman->changeOutputPath(path + n);
    fi.setFile(path + n);
    if (fi.isDir() && !fi.absoluteFilePath().endsWith(bt::DirSeparator())) {
        outputdir = stats.output_path = path + n + bt::DirSeparator();
    } else {
        outputdir = stats.output_path = path + n;
    }
    istats.custom_output_name = true;
}

void TorrentControl::downloaded(Uint32 chunk)
{
    Q_EMIT chunkDownloaded(this, chunk);
}

void TorrentControl::moveToCompletedDir()
{
    QString outdir = completed_dir;
    if (!outdir.endsWith(bt::DirSeparator()))
        outdir += bt::DirSeparator();

    changeOutputDir(outdir, bt::TorrentInterface::MOVE_FILES);
}

void TorrentControl::setAllowedToStart(bool on)
{
    stats.qm_can_start = on;
    if (on && stats.stopped_by_error) {
        // clear the error so user can restart the torrent
        stats.stopped_by_error = false;
    }
    saveStats();
}

void TorrentControl::setQueued(bool queued)
{
    stats.queued = queued;
    updateStatus();
}

QString TorrentControl::getComments() const
{
    return tor->getComments();
}

void TorrentControl::allJobsDone()
{
    updateStatus();
    // update the QM to be sure
    Q_EMIT updateQueue();
    Q_EMIT runningJobsDone(this);
}

void TorrentControl::setChunkSelector(std::unique_ptr<ChunkSelectorInterface> csel)
{
    if (!downloader)
        return;

    downloader->setChunkSelector(std::move(csel));
}

void TorrentControl::networkUp()
{
    psman->manualUpdate();
    pman->killStalePeers();
}

bool TorrentControl::preallocate()
{
    // only start preallocation if we are allowed by the settings
    if (Cache::preallocationEnabled() && !cman->haveAllChunks()) {
        Out(SYS_GEN | LOG_NOTICE) << "Pre-allocating diskspace" << endl;
        stats.running = true;
        job_queue->enqueue(new PreallocationJob(cman.get(), this));
        updateStatus();
        return true;
    } else {
        prealloc = false;
    }

    return false;
}

void TorrentControl::downloadPriorityChanged(TorrentFile *tf, Priority newpriority, Priority oldpriority)
{
    if (cman)
        cman->downloadPriorityChanged(tf, newpriority, oldpriority);

    // If a file has been reenabled, preallocate it and update stats
    if (oldpriority == EXCLUDED) {
        prealloc = true;
        stats.completed = false;
        updateStatus();
        updateStats();
        // Trigger an update of the queue, so it can be restarted again, if it was auto stopped
        Q_EMIT updateQueue();
    }
}

void TorrentControl::setSuperSeeding(bool on)
{
    if (stats.superseeding == on)
        return;

    stats.superseeding = on;
    if (on) {
        if (stats.running && stats.completed)
            pman->setSuperSeeding(true, cman->getBitSet());
    } else {
        pman->setSuperSeeding(false, cman->getBitSet());
    }

    saveStats();
}

TorrentFileStream::Ptr TorrentControl::createTorrentFileStream(Uint32 index, bool streaming_mode, QObject *parent)
{
    if (streaming_mode && !stream.toStrongRef().isNull())
        return TorrentFileStream::Ptr(nullptr);

    if (stats.multi_file_torrent) {
        if (index >= tor->getNumFiles())
            return TorrentFileStream::Ptr(nullptr);

        TorrentFileStream::Ptr ptr(new TorrentFileStream(this, index, cman.get(), streaming_mode, parent));
        if (streaming_mode)
            stream = ptr;
        return ptr;
    } else {
        TorrentFileStream::Ptr ptr(new TorrentFileStream(this, cman.get(), streaming_mode, parent));
        if (streaming_mode)
            stream = ptr;
        return ptr;
    }
}

}

#include "moc_torrentcontrol.cpp"
