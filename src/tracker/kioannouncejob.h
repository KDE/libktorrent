/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_KIOANNOUNCEJOB_H
#define BT_KIOANNOUNCEJOB_H

#include <QUrl>
#include <kio/jobclasses.h>
#include <ktorrent_export.h>

namespace bt
{
class KTORRENT_EXPORT KIOAnnounceJob : public KIO::Job
{
    Q_OBJECT
public:
    KIOAnnounceJob(const QUrl &url, const KIO::MetaData &md);
    ~KIOAnnounceJob() override;

    /// Get the announce url
    QUrl announceUrl() const
    {
        return url;
    }

    /// Get the reply data
    const QByteArray &replyData() const
    {
        return reply_data;
    }

    bool doKill() override;

    bool IsErrorPage() const
    {
        return error_page;
    }

private Q_SLOTS:
    void data(KIO::Job *j, const QByteArray &data);
    void finished(KJob *j);

private:
    bool error_page;
    QUrl url;
    QByteArray reply_data;
    KIO::TransferJob *get_job;
};

}

#endif // BT_KIOANNOUNCEJOB_H
