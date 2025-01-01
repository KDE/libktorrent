/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTCLI_H
#define KTCLI_H

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTimer>
#include <interfaces/queuemanagerinterface.h>
#include <torrent/torrentcontrol.h>

class QUrl;

typedef std::unique_ptr<bt::TorrentControl> TorrentControlPtr;

class KTCLI : public QCoreApplication, public bt::QueueManagerInterface
{
    Q_OBJECT
public:
    KTCLI(int argc, char **argv);
    ~KTCLI() override;

    /// Start downloading
    bool start();

private:
    QString tempDir();
    bool load(const QUrl &url);
    bool loadFromFile(const QString &path);
    bool loadFromDir(const QString &path);

    bool notify(QObject *obj, QEvent *ev) override;
    bool alreadyLoaded(const bt::SHA1Hash &ih) const override;
    void mergeAnnounceList(const bt::SHA1Hash &ih, const bt::TrackerTier *trk) override;

public Q_SLOTS:
    void update();
    void finished(bt::TorrentInterface *tor);
    void shutdown();

private:
    QCommandLineParser parser;
    QTimer timer;
    TorrentControlPtr tc;
    int updates;
};

#endif // KTCLI_H
