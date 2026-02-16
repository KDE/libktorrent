/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kioannouncejob.h"
#include <KIO/Job>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
KIOAnnounceJob::KIOAnnounceJob(const QUrl &url, const KIO::MetaData &md)
    : url(url)
{
    get_job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
    get_job->setMetaData(md);
    connect(get_job, &KIO::TransferJob::data, this, &KIOAnnounceJob::data);
    connect(get_job, &KIO::TransferJob::result, this, &KIOAnnounceJob::finished);
}

KIOAnnounceJob::~KIOAnnounceJob()
{
}

void KIOAnnounceJob::data(KIO::Job *j, const QByteArray &data)
{
    const int MAX_REPLY_SIZE = 1024 * 1024;
    Q_UNUSED(j);
    if (reply_data.size() + data.size() > MAX_REPLY_SIZE) {
        // If the reply is larger then a mega byte, the server
        // has probably gone bonkers
        get_job->kill();
        setError(KIO::ERR_ABORTED);
        Out(SYS_TRK | LOG_DEBUG) << "Tracker sending back to much data in announce reply, aborting ..." << endl;
        emitResult();
    } else {
        reply_data.append(data);
    }
}

bool KIOAnnounceJob::doKill()
{
    get_job->kill();
    return KIO::Job::doKill();
}

void KIOAnnounceJob::finished(KJob *j)
{
    setError(j->error());
    setErrorText(j->errorText());

    emitResult();
}
}

#include "moc_kioannouncejob.cpp"
