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

#include <QTimer>
#include <KApplication>
#include <boost/scoped_ptr.hpp>
#include <torrent/torrentcontrol.h>
#include <interfaces/queuemanagerinterface.h>

class QUrl;

typedef boost::scoped_ptr<bt::TorrentControl> TorrentControlPtr;

class KTCLI : public KApplication,public bt::QueueManagerInterface
{
	Q_OBJECT
public:
	KTCLI();
	virtual ~KTCLI();
	
	/// Start downloading
	bool start();
	
private:
	QString tempDir();
	bool load(const QUrl &url);
	bool loadFromFile(const QString & path);
	bool loadFromDir(const QString & path);
	
	virtual bool notify(QObject* obj, QEvent* ev);
	virtual bool alreadyLoaded(const bt::SHA1Hash& ih) const;
	virtual void mergeAnnounceList(const bt::SHA1Hash& ih, const bt::TrackerTier* trk);
	
public slots:
	void update();
	void finished(bt::TorrentInterface* tor);
	void shutdown();
	
private:
	QTimer timer;
	TorrentControlPtr tc;
	int updates;
};

#endif // KTCLI_H
