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

#include "datacheckerjob.h"
#include "datacheckerthread.h"
#include "multidatachecker.h"
#include "singledatachecker.h"
#include <torrent/torrentcontrol.h>
#include <util/functions.h>
#include <KLocale>

namespace bt
{
	static ResourceManager data_checker_slot(1);
	
	DataCheckerJob::DataCheckerJob(bool auto_import, bt::TorrentControl* tc, bt::Uint32 from, bt::Uint32 to)
		: Job(true,tc),
		Resource(&data_checker_slot,tc->getInfoHash().toString()),
		dcheck_thread(0),
		killed(false),
		auto_import(auto_import),
		started(false),
		from(from),
		to(to)
	{
	}
	
	
	DataCheckerJob::~DataCheckerJob()
	{
	}

	void DataCheckerJob::start()
	{
		registerWithTracker();
		DataChecker* dc = 0;
		const TorrentStats & stats = torrent()->getStats();
		if (stats.multi_file_torrent)
			dc = new MultiDataChecker(from, to);
		else
			dc = new SingleDataChecker(from, to);
		
		connect(dc,SIGNAL(progress(quint32,quint32)),
				this,SLOT(progress(quint32,quint32)),Qt::QueuedConnection);
		connect(dc,SIGNAL(status(quint32,quint32,quint32,quint32)),
				this,SLOT(status(quint32,quint32,quint32,quint32)),Qt::QueuedConnection);
		
		TorrentControl* tor = torrent();
		dcheck_thread = new DataCheckerThread(
				dc,tor->downloadedChunksBitSet(),
				stats.output_path,tor->getTorrent(),
				tor->getTorDir() + "dnd" + bt::DirSeparator());
				
		connect(dcheck_thread,SIGNAL(finished()),this,SLOT(threadFinished()),Qt::QueuedConnection);
		
		torrent()->beforeDataCheck();
		
		setTotalAmount(Bytes,stats.total_chunks);
		data_checker_slot.add(this);
		if (!started)
			infoMessage(this,i18n("Waiting for other data checks to finish"));
	}
	
	void DataCheckerJob::acquired()
	{
		started = true;
		description(this,i18n("Checking data"));
		dcheck_thread->start(QThread::IdlePriority);
	}

	void DataCheckerJob::kill(bool quietly)
	{
		killed = true;
		if (dcheck_thread && dcheck_thread->isRunning())
		{
			dcheck_thread->getDataChecker()->stop();
			dcheck_thread->wait();
			dcheck_thread->deleteLater();
			dcheck_thread = 0;
		}
		bt::Job::kill(quietly);
	}
	
	
	void DataCheckerJob::threadFinished()
	{
		if (!killed)
		{
			DataChecker* dc = dcheck_thread->getDataChecker();
			torrent()->afterDataCheck(this,dc->getResult());
			if (!dcheck_thread->getError().isEmpty())
			{
				setErrorText(dcheck_thread->getError());
				setError(KIO::ERR_UNKNOWN);
			}
			else
				setError(0);
		}
		else
			setError(0);
		
		dcheck_thread->deleteLater();
		dcheck_thread = 0;
		if (!killed) // Job::kill already emitted the result
			emitResult();
		
		release();
	}
	
	void DataCheckerJob::progress(quint32 num, quint32 total)
	{
		Q_UNUSED(total);
		setProcessedAmount(Bytes,num);
	}
	
	void DataCheckerJob::status(quint32 num_failed, quint32 num_found, quint32 num_downloaded, quint32 num_not_downloaded)
	{
		QPair<QString,QString> field1 = qMakePair(QString::number(num_failed),QString::number(num_found));
		QPair<QString,QString> field2 = qMakePair(QString::number(num_downloaded),QString::number(num_not_downloaded));
		description(this,i18n("Checking Data"),field1,field2);
	}
}