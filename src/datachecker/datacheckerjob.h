/***************************************************************************
 *   Copyright (C) 2009 by                                                 *
 *   Joris Guisson <joris.guisson@gmail.com>                               *
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
#ifndef BT_DATACHECKERJOB_H
#define BT_DATACHECKERJOB_H

#include <torrent/job.h>

namespace bt 
{
	class DataCheckerThread;

	
	/// Job which runs a DataChecker
	class KTORRENT_EXPORT DataCheckerJob : public bt::Job
	{
		Q_OBJECT
	public:
		DataCheckerJob(bool auto_import,TorrentControl* tc);
		virtual ~DataCheckerJob();
		
		virtual void start();
		virtual void kill(bool quietly = true);
		virtual TorrentStatus torrentStatus() const {return CHECKING_DATA;}
		
		/// Is this an automatic import
		bool isAutoImport() const {return auto_import;}
		
		/// Was the job stopped
		bool isStopped() const {return killed;}
		
	private slots:
		void threadFinished();
		void progress(quint32 num, quint32 total);
		void status(quint32 num_failed, quint32 num_found, quint32 num_downloaded, quint32 num_not_downloaded);
		
	private:
		DataCheckerThread* dcheck_thread;
		bool killed;
		bool auto_import;
	};

}

#endif // BT_DATACHECKERJOB_H
