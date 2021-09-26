/*
    SPDX-FileCopyrightText: 2009 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef NET_REVERSERESOLVER_H
#define NET_REVERSERESOLVER_H

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QWaitCondition>
#include <ktorrent_export.h>
#include <net/address.h>

namespace net
{
class ReverseResolverThread;

/**
    Resolve an IP address into a hostname
    This should be threated as fire and forget objects, when using them asynchronously.
    The worker thread will delete them, when they are done.
*/
class ReverseResolver : public QObject
{
    Q_OBJECT
public:
    ReverseResolver(QObject *parent = nullptr);
    ~ReverseResolver() override;

    /**
        Resolve an ip address asynchronously, uses the worker thread
        Connecting to the resolved signal should be done with Qt::QueuedConnection, seeing
        that it will be emitted from the worker thread.
        @param addr The address
    */
    void resolveAsync(const net::Address &addr);

    /**
        Resolve an ip address synchronously.
    */
    QString resolve(const net::Address &addr);

    /**
        Run the actual resolve and emit the signal when done
    */
    void run();

    /// Shutdown the worker thread
    static void shutdown();

Q_SIGNALS:
    /// Emitted when the resolution is complete
    void resolved(const QString &host);

private:
    static ReverseResolverThread *worker;
    net::Address addr_to_resolve;
};

class ReverseResolverThread : public QThread
{
    Q_OBJECT
public:
    ReverseResolverThread();
    ~ReverseResolverThread() override;

    /// Add a ReverseResolver to the todo list
    void add(ReverseResolver *rr);

    /// Run the thread
    void run() override;

    /// Stop the thread
    void stop();

private:
    QMutex mutex;
    QWaitCondition more_data;
    QList<ReverseResolver *> todo_list;
    bool stopped;
};

}

#endif // NET_REVERSERESOLVER_H
