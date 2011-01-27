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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include "socketmonitor.h"
#include <math.h>
#include <unistd.h>
#include <util/functions.h>
#include <util/log.h>
#include <torrent/globals.h>
#include "bufferedsocket.h"
#include "uploadthread.h"
#include "downloadthread.h"

using namespace bt;

namespace net
{
	SocketMonitor SocketMonitor::self;
	
	class SocketMonitor::Private
	{
	public:
		Private(SocketMonitor* p) : mutex(QMutex::Recursive),ut(0),dt(0),next_group_id(1)
		{
			dt = new DownloadThread(p);
			ut = new UploadThread(p);
		}
		
		~Private()
		{
			shutdown();
		}
		
		void shutdown();
		
		QMutex mutex;
		UploadThread* ut;
		DownloadThread* dt;
		Uint32 next_group_id;
	};

	SocketMonitor::SocketMonitor() : d(new Private(this))
	{
		
	}


	SocketMonitor::~SocketMonitor()
	{
		delete d;
	}
	
	void SocketMonitor::shutdown()
	{
		d->shutdown();
	}
	
	void SocketMonitor::Private::shutdown()
	{
		if (ut && ut->isRunning())
		{
			ut->stop();
			ut->signalDataReady(); // kick it in the nuts, if the thread is waiting for data
			if (!ut->wait(250))
			{
				ut->terminate();
				ut->wait();
			}
		}
		
		
		if (dt && dt->isRunning())
		{
			dt->stop();
			dt->wakeUp(); // wake it up if necessary
			if (!dt->wait(250))
			{
				dt->terminate();
				dt->wait();
			}
		}
		
		delete ut;
		delete dt;
		ut = 0;
		dt = 0;
	}

	
	void SocketMonitor::lock()
	{
		d->mutex.lock();
	}
	
	void SocketMonitor::unlock()
	{
		d->mutex.unlock();
	}
	
	void SocketMonitor::setDownloadCap(Uint32 bytes_per_sec)
	{
		DownloadThread::setCap(bytes_per_sec);
	}
	
	Uint32 SocketMonitor::getDownloadCap()
	{
		return DownloadThread::cap();
	}

	
	void SocketMonitor::setUploadCap(Uint32 bytes_per_sec)
	{
		UploadThread::setCap(bytes_per_sec);
	}
	
	Uint32 SocketMonitor::getUploadCap()
	{
		return UploadThread::cap();
	}

	
	void SocketMonitor::setSleepTime(Uint32 sleep_time)
	{
		DownloadThread::setSleepTime(sleep_time);
		UploadThread::setSleepTime(sleep_time);
	}
	
	void SocketMonitor::add(BufferedSocket* sock)
	{
		QMutexLocker lock(&d->mutex);
		
		if (!d->dt || !d->ut)
			return;
		
		bool start_threads = sockets.size() == 0;
		sockets.push_back(sock);
		
		if (start_threads)
		{
			Out(SYS_CON|LOG_DEBUG) << "Starting socketmonitor threads" << endl;
				
			if (!d->dt->isRunning())
				d->dt->start(QThread::IdlePriority);
			if (!d->ut->isRunning())
				d->ut->start(QThread::IdlePriority);
		}
		// wake up download thread so that it can start polling the new socket
		d->dt->wakeUp();
	}
	
	void SocketMonitor::remove(BufferedSocket* sock)
	{
		QMutexLocker lock(&d->mutex);
		if (sockets.size() == 0)
			return;
		
		sockets.remove(sock);
	}
	
	void SocketMonitor::signalPacketReady()
	{
		if (d->ut)
			d->ut->signalDataReady();
	}
	
	Uint32 SocketMonitor::newGroup(GroupType type,Uint32 limit,Uint32 assured_rate)
	{
		QMutexLocker lock(&d->mutex);
		if (!d->dt || !d->ut)
			return 0;
		
		Uint32 gid = d->next_group_id++;
		if (type == UPLOAD_GROUP)
			d->ut->addGroup(gid,limit,assured_rate);
		else
			d->dt->addGroup(gid,limit,assured_rate);
		
		return gid;
	}
		
	void SocketMonitor::setGroupLimit(GroupType type,Uint32 gid,Uint32 limit)
	{
		QMutexLocker lock(&d->mutex);
		if (!d->dt || !d->ut)
			return;
		
		if (type == UPLOAD_GROUP)
			d->ut->setGroupLimit(gid,limit);
		else
			d->dt->setGroupLimit(gid,limit);
	}
	
	void SocketMonitor::setGroupAssuredRate(GroupType type,Uint32 gid,Uint32 as)
	{
		QMutexLocker lock(&d->mutex);
		if (!d->dt || !d->ut)
			return;
		
		if (type == UPLOAD_GROUP)
			d->ut->setGroupAssuredRate(gid,as);
		else
			d->dt->setGroupAssuredRate(gid,as);
	}
		
	void SocketMonitor::removeGroup(GroupType type,Uint32 gid)
	{
		QMutexLocker lock(&d->mutex);
		if (!d->dt || !d->ut)
			return;
		
		if (type == UPLOAD_GROUP)
			d->ut->removeGroup(gid);
		else
			d->dt->removeGroup(gid);
	}

}
