/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTTASKMANAGER_H
#define DHTTASKMANAGER_H

#include "task.h"
#include <QList>
#include <QPointer>
#include <util/constants.h>

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
public:
    TaskManager(const DHT *dh_table);
    ~TaskManager() override;

    /**
     * Add a task to manage.
     * @param task
     */
    void addTask(Task *task);

    /// Get the number of running tasks
    bt::Uint32 getNumTasks() const
    {
        return num_active;
    }

    /// Get the number of queued tasks
    bt::Uint32 getNumQueuedTasks() const
    {
        return queued.count();
    }

private:
    void taskFinished(Task *task);

    const DHT *dh_table;
    QList<QPointer<Task>> queued;
    bt::Uint32 num_active;
};

}

#endif
