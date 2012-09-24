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
#include "preallocationthread.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <util/log.h>
#include <util/error.h>
#include <qfile.h>
#include <klocale.h>
#include "chunkmanager.h"

#ifndef O_LARGEFILE
# define O_LARGEFILE 0
#endif

namespace bt
{

	PreallocationThread::PreallocationThread() : 
		stopped(false), 
		not_finished(false), 
		done(false),
		bytes_written(0)
	{
	}


	PreallocationThread::~PreallocationThread()
	{}
	
	void PreallocationThread::add(CacheFile::Ptr cache_file)
	{
		if(cache_file)
			todo.append(cache_file);
	}


	void PreallocationThread::run()
	{
		try
		{
			foreach(CacheFile::Ptr cache_file, todo)
			{
				if(!isStopped())
				{
					cache_file->preallocate(this);
				}
				else
				{
					setNotFinished();
					break;
				}
			}
				
		}
		catch(Error & err)
		{
			setErrorMsg(err.toString());
		}

		QMutexLocker lock(&mutex);
		done = true;
		Out(SYS_DIO | LOG_NOTICE) << "PreallocationThread has finished" << endl;
	}

	void PreallocationThread::stop()
	{
		QMutexLocker lock(&mutex);
		stopped = true;
	}

	void PreallocationThread::setErrorMsg(const QString & msg)
	{
		QMutexLocker lock(&mutex);
		error_msg = msg; 
		stopped = true;
	}

	bool PreallocationThread::isStopped() const
	{
		QMutexLocker lock(&mutex);
		return stopped;
	}

	bool PreallocationThread::errorHappened() const
	{
		QMutexLocker lock(&mutex);
		return !error_msg.isNull();
	}

	void PreallocationThread::written(Uint64 nb)
	{
		QMutexLocker lock(&mutex);
		bytes_written += nb;
	}

	Uint64 PreallocationThread::bytesWritten()
	{
		QMutexLocker lock(&mutex);
		return bytes_written;
	}

	bool PreallocationThread::isDone() const
	{
		QMutexLocker lock(&mutex);
		return done;
	}

	bool PreallocationThread::isNotFinished() const
	{
		QMutexLocker lock(&mutex);
		return not_finished;
	}

	void PreallocationThread::setNotFinished()
	{
		QMutexLocker lock(&mutex);
		not_finished = true;
	}
}
