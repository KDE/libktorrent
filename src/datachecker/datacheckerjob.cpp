/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "datacheckerjob.h"
#include "datacheckerthread.h"
#include "multidatachecker.h"
#include "singledatachecker.h"
#include <KIO/Global>
#include <klocalizedstring.h>
#include <torrent/torrentcontrol.h>
#include <util/functions.h>

namespace bt
{
static ResourceManager data_checker_slot(1);

DataCheckerJob::DataCheckerJob(bool auto_import, bt::TorrentControl *tc, bt::Uint32 from, bt::Uint32 to)
    : Job(true, tc)
    , Resource(&data_checker_slot, tc->getInfoHash().toString())
    , dcheck_thread(nullptr)
    , killed(false)
    , auto_import(auto_import)
    , started(false)
    , from(from)
    , to(to)
{
    if (this->from >= tc->getStats().total_chunks)
        this->from = 0;
    if (this->to >= tc->getStats().total_chunks)
        this->to = tc->getStats().total_chunks - 1;
}

DataCheckerJob::~DataCheckerJob()
{
}

void DataCheckerJob::start()
{
    registerWithTracker();
    DataChecker *dc = nullptr;
    const TorrentStats &stats = torrent()->getStats();
    if (stats.multi_file_torrent)
        dc = new MultiDataChecker(from, to);
    else
        dc = new SingleDataChecker(from, to);

    connect(dc, &DataChecker::progress, this, &DataCheckerJob::progress, Qt::QueuedConnection);
    connect(dc, &DataChecker::status, this, &DataCheckerJob::status, Qt::QueuedConnection);

    TorrentControl *tor = torrent();
    dcheck_thread = new DataCheckerThread(dc, //
                                          tor->downloadedChunksBitSet(),
                                          stats.output_path,
                                          tor->getTorrent(),
                                          tor->getTorDir() + "dnd" + bt::DirSeparator());

    connect(dcheck_thread, &DataCheckerThread::finished, this, &DataCheckerJob::threadFinished, Qt::QueuedConnection);

    torrent()->beforeDataCheck();

    setTotalAmount(Bytes, to - from + 1);
    data_checker_slot.add(this);
    if (!started)
        infoMessage(this, i18n("Waiting for other data checks to finish"));
}

void DataCheckerJob::acquired()
{
    started = true;
    description(this, i18n("Checking data"));
    dcheck_thread->start(QThread::IdlePriority);
}

void DataCheckerJob::kill(bool quietly)
{
    killed = true;
    if (dcheck_thread && dcheck_thread->isRunning()) {
        dcheck_thread->getDataChecker()->stop();
        dcheck_thread->wait();
        dcheck_thread->deleteLater();
        dcheck_thread = nullptr;
    }
    bt::Job::kill(quietly);
}

void DataCheckerJob::threadFinished()
{
    if (!killed) {
        DataChecker *dc = dcheck_thread->getDataChecker();
        torrent()->afterDataCheck(this, dc->getResult());
        if (!dcheck_thread->getError().isEmpty()) {
            setErrorText(dcheck_thread->getError());
            setError(KIO::ERR_UNKNOWN);
        } else
            setError(0);
    } else
        setError(0);

    dcheck_thread->deleteLater();
    dcheck_thread = nullptr;
    if (!killed) // Job::kill already emitted the result
        emitResult();

    release();
}

void DataCheckerJob::progress(quint32 num, quint32 total)
{
    Q_UNUSED(total);
    setProcessedAmount(Bytes, num);
}

void DataCheckerJob::status(quint32 num_failed, quint32 num_found, quint32 num_downloaded, quint32 num_not_downloaded)
{
    QPair<QString, QString> field1 = qMakePair(QString::number(num_failed), QString::number(num_found));
    QPair<QString, QString> field2 = qMakePair(QString::number(num_downloaded), QString::number(num_not_downloaded));
    description(this, i18n("Checking Data"), field1, field2);
}
}
