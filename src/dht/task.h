/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTTASK_H
#define DHTTASK_H

#include "kbucket.h"
#include "rpccall.h"
#include "rpcserver.h"

namespace net
{
class AddressResolver;
}

namespace dht
{
class Node;
class Task;
class KClosestNodesSearch;

const bt::Uint32 MAX_CONCURRENT_REQS = 16;

/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Performs a task on K nodes provided by a KClosestNodesSearch.
 * This is a base class for all tasks.
 */
class Task : public RPCCallListener
{
    Q_OBJECT
public:
    /**
     * Create a task.
     * @param rpc The RPC server to do RPC calls
     * @param node The node
     * @param parent The parent object
     */
    Task(RPCServer *rpc, Node *node, QObject *parent);
    ~Task() override;

    /**
     * This will copy the results from the KClosestNodesSearch
     * object into the todo list. And call update if the task is not queued.
     * @param kns The KClosestNodesSearch object
     * @param queued Is the task queued
     */
    void start(const KClosestNodesSearch &kns, bool queued);

    /**
     *  Start the task, to be used when a task is queued.
     */
    void start();

    /// Decrements the outstanding_reqs
    void onResponse(RPCCall *c, RPCMsg::Ptr rsp) override;

    /// Decrements the outstanding_reqs
    void onTimeout(RPCCall *c) override;

    /**
     * Will continue the task, this will be called every time we have
     * rpc slots available for this task. Should be implemented by derived classes.
     */
    virtual void update() = 0;

    /**
     * A call is finished and a response was received.
     * @param c The call
     * @param rsp The response
     */
    virtual void callFinished(RPCCall *c, RPCMsg::Ptr rsp) = 0;

    /**
     * A call timedout
     * @param c The call
     */
    virtual void callTimeout(RPCCall *c) = 0;

    /**
     * Do a call to the rpc server, increments the outstanding_reqs variable.
     * @param req THe request to send
     * @return true if call was made, false if not
     */
    bool rpcCall(RPCMsg::Ptr req);

    /// See if we can do a request
    bool canDoRequest() const
    {
        return outstanding_reqs < MAX_CONCURRENT_REQS;
    }

    /// Is the task finished
    bool isFinished() const
    {
        return task_finished;
    }

    /// Get the number of outstanding requests
    bt::Uint32 getNumOutstandingRequests() const
    {
        return outstanding_reqs;
    }

    bool isQueued() const
    {
        return queued;
    }

    /**
     * Tell listeners data is ready.
     */
    void emitDataReady();

    /// Kills the task
    void kill();

    /**
     * Add a node to the todo list
     * @param ip The ip or hostname of the node
     * @param port The port
     */
    void addDHTNode(const QString &ip, bt::Uint16 port);

Q_SIGNALS:
    /**
     * The task is finsihed.
     * @param t The Task
     */
    void finished(Task *t);

    /**
     * Called by the task when data is ready.
     * Can be overrided if wanted.
     * @param t The Task
     */
    void dataReady(Task *t);

protected:
    void done();

protected Q_SLOTS:
    void onResolverResults(net::AddressResolver *res);

protected:
    dht::KBucketEntrySet visited; // nodes visited
    dht::KBucketEntrySet todo; // nodes todo
    Node *node;

private:
    RPCServer *rpc;
    bt::Uint32 outstanding_reqs;
    bool task_finished;
    bool queued;
};

}

#endif
