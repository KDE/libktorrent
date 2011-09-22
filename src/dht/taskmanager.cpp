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
#include "taskmanager.h"
#include <QtAlgorithms>
#include <util/log.h>
#include "nodelookup.h"
#include "dht.h"

using namespace bt;

namespace dht
{

	TaskManager::TaskManager(const DHT* dh_table) : dh_table(dh_table)
	{
	}


	TaskManager::~TaskManager()
	{
	}


	void TaskManager::addTask(Task* task)
	{
		connect(task, SIGNAL(finished(Task*)), this, SLOT(taskFinished(Task*)));
		if (task->isQueued())
			queued.append(task);
		else
			active.append(task);
	}

	void TaskManager::taskFinished(Task* task)
	{
		active.removeAll(task);
		task->deleteLater();

		while (dh_table->canStartTask() && queued.count() > 0)
		{
			Task* t = queued.first();
			queued.removeFirst();
			Out(SYS_DHT | LOG_NOTICE) << "DHT: starting queued task" << endl;
			t->start();
			active.append(t);
		}
	}

}
