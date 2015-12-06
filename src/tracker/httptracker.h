/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#ifndef BTHTTPTRACKER_H
#define BTHTTPTRACKER_H

#include <QTimer>
#include <ktorrent_export.h>
#include "tracker.h"


class KJob;

namespace KIO
{
	class MetaData;
}

namespace bt
{
	

	/**
	 * @author Joris Guisson
	 * @brief Communicates with the tracker
	 *
	 * This class uses the HTTP protocol to communicate with the tracker.
	 */
	class KTORRENT_EXPORT HTTPTracker : public Tracker
	{
		Q_OBJECT
	public:
		HTTPTracker(const QUrl &url,TrackerDataSource* tds,const PeerID & id,int tier);
		virtual ~HTTPTracker();
		
		virtual void start();
		virtual void stop(WaitJob* wjob = 0);
		virtual void completed();
		virtual Uint32 failureCount() const {return failures;}
		virtual void scrape();
		
		static void setProxy(const QString & proxy,const bt::Uint16 proxy_port);
		static void setProxyEnabled(bool on);
		static void setUseQHttp(bool on);
		
	private slots:
		void onKIOAnnounceResult(KJob* j);
#ifdef HAVE_HTTPANNOUNEJOB
		void onQHttpAnnounceResult(KJob* j);
#endif
		void onScrapeResult(KJob* j);
		void emitInvalidURLFailure();
		void onTimeout();
		virtual void manualUpdate();

	private:
		void doRequest(WaitJob* wjob = 0);
		bool updateData(const QByteArray & data);
		void setupMetaData(KIO::MetaData & md);
 		void doAnnounceQueue();
 		void doAnnounce(const QUrl &u);
		void onAnnounceResult(const QUrl &url,const QByteArray & data,KJob* j);

	private:
		KJob* active_job;
		QList<QUrl> announce_queue;
		QString event;
		Uint32 failures;
		QTimer timer;
		QString error;
		bool supports_partial_seed_extension;
		
		
		static bool proxy_on;
		static QString proxy;
		static Uint16 proxy_port;
		static bool use_qhttp;
	};

}

#endif
