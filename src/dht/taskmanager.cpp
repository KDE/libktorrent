/***************************************************************************
 *   Copyright (C) 2012 by                                                 *
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
#include "taskmanager.h"
#include <QtAlgorithms>
#include <util/log.h>
#include "nodelookup.h"
#include "dht.h"

using namespace bt;

namespace dht
{

	TaskManager::TaskManager(const DHT* dh_table) : dh_table(dh_table),num_active(0)
	{
	}

	TaskManager::~TaskManager()
	{
	}

	void TaskManager::addTask(Task* task)
	{
		connect(task, SIGNAL(finished(Task*)), this, SLOT(taskFinished(Task*)));
		if (task->isQueued())
			queued.append(QPointer<Task>(task));
		else
			num_active++;
	}

	void TaskManager::taskFinished(Task* task)
	{
		if (!task->isQueued() && num_active > 0)
			num_active--;
		task->deleteLater();

		while (dh_table->canStartTask() && !queued.isEmpty())
		{
			QPointer<Task> t = queued.takeFirst();
			if (t)
			{
				Out(SYS_DHT | LOG_NOTICE) << "DHT: starting queued task" << endl;
				t.data()->start();
				num_active++;
			}
		}
	}

}
