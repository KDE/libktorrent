/***************************************************************************
 *   Copyright (C) 2009 by Joris Guisson                                   *
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

#ifndef BT_TRACKERMANAGER_H
#define BT_TRACKERMANAGER_H

#include <QTimer>
#include <QDateTime>
#include <ktorrent_export.h>
#include <util/ptrmap.h>
#include <util/constants.h>
#include <tracker/tracker.h>
#include <interfaces/trackerslist.h>


namespace bt 
{
	class TorrentControl;
	class WaitJob;
	class PeerManager;
	
	/**
	 * Manages all trackers
	 */
	class KTORRENT_EXPORT TrackerManager : public QObject,public bt::TrackersList,public TrackerDataSource
	{
		Q_OBJECT
	public:
		TrackerManager(TorrentControl* tor,PeerManager* pman);
		~TrackerManager() override;
		
		TrackerInterface* getCurrentTracker() const override;
		void setCurrentTracker(TrackerInterface* t) override;
		void setCurrentTracker(const QUrl &url) override;
		QList<TrackerInterface*> getTrackers() override;
		TrackerInterface* addTracker(const QUrl &url, bool custom = true,int tier = 1) override;
		bool removeTracker(TrackerInterface* t) override;
		bool removeTracker(const QUrl &url) override;
		bool canRemoveTracker(TrackerInterface* t) override;
		void restoreDefault() override;
		void setTrackerEnabled(const QUrl &url,bool on) override;
		bool noTrackersReachable() const override;
		TrackersStatusInfo getTrackersStatusInfo() const override;
		
		/// Get the number of seeders
		Uint32 getNumSeeders() const;
		
		/// Get the number of leechers
		Uint32 getNumLeechers() const;
		
		/**
		* Start gathering peers
		*/
		virtual void start();
		
		/**
		* Stop gathering peers
		* @param wjob WaitJob to wait at exit for the completion of stopped events to the trackers
		*/
		virtual void stop(WaitJob* wjob = 0);
		
		/**
		* Notify peersources and trackrs that the download is complete.
		*/
		virtual void completed();
		
		/**
		* Do a manual update on all peer sources and trackers.
		*/
		virtual void manualUpdate();
		
		/**
		* Do a scrape on the current tracker
		* */
		virtual void scrape();
		
	protected:
		void saveCustomURLs();
		void loadCustomURLs();
		void saveTrackerStatus();
		void loadTrackerStatus();
		void addTracker(Tracker* trk);
		void switchTracker(Tracker* trk);
		Tracker* selectTracker();
		
		Uint64 bytesDownloaded() const override;
		Uint64 bytesUploaded() const override;
		Uint64 bytesLeft() const override;
		const SHA1Hash & infoHash() const override;
		bool isPartialSeed() const override;
		
	private Q_SLOTS:
		/**
		* The an error happened contacting the tracker.
		* @param err The error
		*/
		void onTrackerError(const QString & err);
		
		/**
		* Tracker update was OK.
		* @param 
		*/
		void onTrackerOK();
		
		/**
		* Update the current tracker manually
		*/
		void updateCurrentManually();
		
	protected:
		TorrentControl* tor;
		PtrMap<QUrl,Tracker> trackers;
		bool no_save_custom_trackers;
		PeerManager* pman;
		Tracker* curr;
		QList<QUrl> custom_trackers;
		bool started;
	};

}

#endif // BT_TRACKERMANAGER_H
