/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "taskmanager.h"
#include "dht.h"
#include "nodelookup.h"
#include <QtAlgorithms>
#include <util/log.h>

using namespace bt;

namespace dht
{
TaskManager::TaskManager(const DHT *dh_table)
    : dh_table(dh_table)
    , num_active(0)
{
}

TaskManager::~TaskManager()
{
}

void TaskManager::addTask(Task *task)
{
    connect(task, &Task::finished, this, &TaskManager::taskFinished);
    if (task->isQueued()) {
        queued.append(QPointer<Task>(task));
    } else {
        num_active++;
    }
}

void TaskManager::taskFinished(Task *task)
{
    if (!task->isQueued() && num_active > 0) {
        num_active--;
    }
    task->deleteLater();

    while (dh_table->canStartTask() && !queued.isEmpty()) {
        QPointer<Task> t = queued.takeFirst();
        if (t) {
            Out(SYS_DHT | LOG_NOTICE) << "DHT: starting queued task" << endl;
            t.data()->start();
            num_active++;
        }
    }
}

}
