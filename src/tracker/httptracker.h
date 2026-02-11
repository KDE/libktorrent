/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTHTTPTRACKER_H
#define BTHTTPTRACKER_H

#include "tracker.h"
#include <QTimer>
#include <ktorrent_export.h>

class KJob;

namespace KIO
{
class MetaData;
}

namespace bt
{
/*!
 * \author Joris Guisson
 * \brief Communicates with a HTTP tracker.
 *
 * This class uses the HTTP protocol to communicate with the tracker.
 */
class KTORRENT_EXPORT HTTPTracker : public Tracker
{
    Q_OBJECT
public:
    HTTPTracker(const QUrl &url, TrackerDataSource *tds, const PeerID &id, int tier);
    ~HTTPTracker() override;

    void start() override;
    void stop(WaitJob *wjob = nullptr) override;
    void completed() override;
    [[nodiscard]] Uint32 failureCount() const override
    {
        return failures;
    }
    void scrape() override;

    static void setProxy(const QString &proxy, const bt::Uint16 proxy_port);
    static void setProxyEnabled(bool on);
    [[deprecated]] static void setUseQHttp(bool on);

private Q_SLOTS:
    void onKIOAnnounceResult(const KJob *j);
    void onScrapeResult(const KJob *j);
    void emitInvalidURLFailure();
    void onTimeout();
    void manualUpdate() override;

private:
    void doRequest(WaitJob *wjob = nullptr);
    bool updateData(const QByteArray &data);
    void setupMetaData(KIO::MetaData &md);
    void doAnnounceQueue();
    void doAnnounce(const QUrl &u);
    void onAnnounceResult(const QUrl &url, const QByteArray &data, const KJob *j);

private:
    KJob *active_job;
    QList<QUrl> announce_queue;
    QString event;
    QTimer timer;
    QString error;
    Uint32 failures;
    bool supports_partial_seed_extension;

    static bool proxy_on;
    static QString proxy;
    static Uint16 proxy_port;
};

}

#endif
