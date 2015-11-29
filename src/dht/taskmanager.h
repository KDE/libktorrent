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
#ifndef DHTTASKMANAGER_H
#define DHTTASKMANAGER_H

#include <QList>
#include <QPointer>
#include <util/constants.h>
#include "task.h"

namespace dht
{
	class DHT;

	/**
	 * @author Joris Guisson <joris.guisson@gmail.com>
	 * 
	 * Manages all dht tasks.
	*/
	class TaskManager : public QObject
	{
		Q_OBJECT
	public:
		TaskManager(const DHT* dh_table);
		virtual ~TaskManager();
		
		/**
		 * Add a task to manage.
		 * @param task 
		 */
		void addTask(Task* task);
		
		/// Get the number of running tasks
		bt::Uint32 getNumTasks() const {return num_active;}
		
		/// Get the number of queued tasks
		bt::Uint32 getNumQueuedTasks() const {return queued.count();}
		
	private slots:
		void taskFinished(Task* task);

	private:
		const DHT* dh_table;
		QList<QPointer<Task> > queued;
		bt::Uint32 num_active;
	};

}

#endif
