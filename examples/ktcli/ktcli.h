/***************************************************************************
 *   Copyright (C) 2010 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef KTCLI_H
#define KTCLI_H

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTimer>
#include <boost/scoped_ptr.hpp>
#include <interfaces/queuemanagerinterface.h>
#include <torrent/torrentcontrol.h>

class QUrl;

typedef boost::scoped_ptr<bt::TorrentControl> TorrentControlPtr;

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
