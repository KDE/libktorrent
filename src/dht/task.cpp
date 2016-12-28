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
#include "task.h"
#include <net/addressresolver.h>
#include "kclosestnodessearch.h"
#include "rpcserver.h"

namespace dht
{

	Task::Task(RPCServer* rpc, Node* node, QObject* parent)
			: RPCCallListener(parent),
			node(node),
			rpc(rpc),
			outstanding_reqs(0),
			task_finished(false),
			queued(true)
	{
	}

	Task::~Task()
	{
	}

	void Task::start(const KClosestNodesSearch & kns, bool queued)
	{
		// Fill the todo list
		for (KClosestNodesSearch::CItr i = kns.begin(); i != kns.end();i++)
			todo.insert(i->second);
		this->queued = queued;
		if (!queued)
			update();
	}

	void Task::start()
	{
		if (queued)
		{
			queued = false;
			update();
		}
	}

	void Task::onResponse(dht::RPCCall* c, dht::RPCMsg::Ptr rsp)
	{
		if (outstanding_reqs > 0)
			outstanding_reqs--;

		if (!isFinished())
		{
			callFinished(c, rsp);

			if (canDoRequest() && !isFinished())
				update();
		}
	}

	void Task::onTimeout(RPCCall* c)
	{
		if (outstanding_reqs > 0)
			outstanding_reqs--;

		if (!isFinished())
		{
			callTimeout(c);

			if (canDoRequest() && !isFinished())
				update();
		}
	}

	bool Task::rpcCall(dht::RPCMsg::Ptr req)
	{
		if (!canDoRequest())
			return false;

		RPCCall* c = rpc->doCall(req);
		c->addListener(this);
		outstanding_reqs++;
		return true;
	}

	void Task::done()
	{
		task_finished = true;
		finished(this);
	}

	void Task::emitDataReady()
	{
		dataReady(this);
	}

	void Task::kill()
	{
		task_finished = true;
		finished(this);
	}

	void Task::addDHTNode(const QString & ip, bt::Uint16 port)
	{
		net::Address addr;
		if (addr.setAddress(ip))
		{
			addr.setPort(port);
			todo.insert(KBucketEntry(addr, dht::Key()));
		}
		else
			net::AddressResolver::resolve(ip, port, this, SLOT(onResolverResults(net::AddressResolver*)));
	}

	void Task::onResolverResults(net::AddressResolver* ar)
	{
		if (!ar->succeeded())
			return;

		todo.insert(KBucketEntry(ar->address(), dht::Key()));
	}

}
