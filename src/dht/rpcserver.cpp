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
#include "rpcserver.h"
#include <QHostAddress>
#include <QThread>
#include <unistd.h>
#include <string.h>
#include <boost/scoped_ptr.hpp>
#include <net/portlist.h>
#include <util/log.h>
#include <util/error.h>
#include <torrent/globals.h>
#include <bcodec/bnode.h>
#include <bcodec/bdecoder.h>
#include <bcodec/bencoder.h>
#include <util/functions.h>
#include <net/serversocket.h>
#include "rpccall.h"
#include "rpcmsg.h"
#include "kbucket.h"
#include "node.h"
#include "dht.h"
#include "pingreq.h"
#include "rpcmsgfactory.h"

using namespace bt;

namespace dht
{

	class RPCServer::Private : public net::ServerSocket::DataHandler, public RPCMethodResolver
	{
	public:
		Private(RPCServer* p, DHT* dh_table, Uint16 port)
				: p(p), dh_table(dh_table), next_mtid(0), port(port)
		{}

		~Private() override
		{
			bt::Globals::instance().getPortList().removePort(port, net::UDP);
			calls.setAutoDelete(true);
			calls.clear();
			qDeleteAll(call_queue);
			call_queue.clear();
		}

		void reset()
		{
			sockets.clear();
		}

		void listen(const QString & ip)
		{
			net::Address addr(ip, port);
			net::ServerSocket::Ptr sock(new net::ServerSocket(this));
			if (!sock->bind(addr))
			{
				Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Failed to bind to " << addr.toString() << endl;
			}
			else
			{
				Out(SYS_DHT | LOG_NOTICE) << "DHT: Bound to " << addr.toString() << endl;
				sockets.append(sock);
				sock->setReadNotificationsEnabled(true);
			}
		}

		void dataReceived(bt::Buffer::Ptr ptr, const net::Address& addr) override
		{
			try
			{
				// read and decode the packet
				BDecoder bdec(ptr->get(), ptr->size(), false);
				boost::scoped_ptr<BNode> n(bdec.decode());

				if (!n || n->getType() != BNode::DICT)
					return;

				// try to make a RPCMsg of it
				RPCMsg::Ptr msg = factory.build((BDictNode*)n.get(), this);
				if (msg)
				{
					if (addr.ipVersion() == 6 && addr.isIPv4Mapped())
						msg->setOrigin(addr.convertIPv4Mapped());
					else
						msg->setOrigin(addr);
					
					msg->apply(dh_table);
					// erase an existing call
					if (msg->getType() == RSP_MSG && calls.contains(msg->getMTID()))
					{
						// delete the call, but first notify it off the response
						RPCCall* c = calls.find(msg->getMTID());
						c->response(msg);
						calls.erase(msg->getMTID());
						c->deleteLater();
						doQueuedCalls();
					}
				}
			}
			catch (bt::Error & err)
			{
				Out(SYS_DHT | LOG_DEBUG) << "Error happened during parsing : " << err.toString() << endl;
			}
		}

		void readyToWrite(net::ServerSocket* sock) override
		{
			Q_UNUSED(sock);
		}
		
		Method findMethod(const QByteArray& mtid) override
		{
			const RPCCall* call = calls.find(mtid);
			if (call)
				return call->getMsgMethod();
			else
				return dht::NONE;
		}

		void send(const net::Address & addr, const QByteArray & msg)
		{
			foreach(net::ServerSocket::Ptr sock, sockets)
			{
				if (sock->sendTo((const bt::Uint8*)msg.data(), msg.size(), addr) == msg.size())
					break;
			}
		}

		void doQueuedCalls()
		{
			while (call_queue.count() > 0 && calls.count() < 256)
			{
				RPCCall* c = call_queue.first();
				call_queue.removeFirst();

				while (calls.contains(QByteArray(1, next_mtid)))
					next_mtid++;

				RPCMsg::Ptr msg = c->getRequest();
				QByteArray mtid(1, next_mtid);
				msg->setMTID(mtid);
				next_mtid++;
				sendMsg(msg);
				calls.insert(msg->getMTID(), c);
				c->start();
			}
		}

		RPCCall* doCall(RPCMsg::Ptr msg)
		{
			Uint8 start = next_mtid;
			QByteArray mtid(1, start);
			
			while (calls.contains(mtid))
			{
				mtid[0] = ++next_mtid;
				if (next_mtid == start) // if this happens we cannot do any calls
				{
					// so queue the call
					RPCCall* c = new RPCCall(msg, true);
					call_queue.append(c);
					Out(SYS_DHT | LOG_NOTICE) << "Queueing RPC call, no slots available at the moment" << endl;
					return c;
				}
			}

			msg->setMTID(mtid);
			sendMsg(msg);
			RPCCall* c = new RPCCall(msg, false);
			calls.insert(mtid, c);
			return c;
		}

		void sendMsg(RPCMsg::Ptr msg)
		{
			QByteArray data;
			msg->encode(data);
			send(msg->getDestination(), data);
			//	PrintRawData(data);
		}

		void timedOut(const QByteArray & mtid)
		{
			// delete the call
			RPCCall* c = calls.find(mtid);
			if (c)
			{
				dh_table->timeout(c->getRequest());
				calls.erase(mtid);
				c->deleteLater();
			}
			doQueuedCalls();
		}

		RPCServer* p;
		QList<net::ServerSocket::Ptr> sockets;
		DHT* dh_table;
		bt::PtrMap<QByteArray, RPCCall> calls;
		QList<RPCCall*> call_queue;
		bt::Uint8 next_mtid;
		bt::Uint16 port;
		RPCMsgFactory factory;
	};



	RPCServer::RPCServer(DHT* dh_table, Uint16 port, QObject *parent)
			: QObject(parent), d(new Private(this, dh_table, port))
	{
	}


	RPCServer::~RPCServer()
	{
		delete d;
	}

	void RPCServer::start()
	{
		d->reset();

		QStringList ips = NetworkInterfaceIPAddresses(NetworkInterface());
		foreach(const QString & addr, ips)
		{
			d->listen(addr);
		}

		if (d->sockets.count() == 0)
		{
			// Try all addresses if the previous listen calls all failed
			d->listen(QHostAddress(QHostAddress::AnyIPv6).toString());
			d->listen(QHostAddress(QHostAddress::Any).toString());
		}

		if (d->sockets.count() > 0)
			bt::Globals::instance().getPortList().addNewPort(d->port, net::UDP, true);
	}

	void RPCServer::stop()
	{
		bt::Globals::instance().getPortList().removePort(d->port, net::UDP);
		d->reset();
	}

#if 0
	static void PrintRawData(const QByteArray & data)
	{
		QString tmp;
		for (int i = 0;i < data.size();i++)
		{
			char c = QChar(data[i]).toLatin1();
			if (!QChar(data[i]).isPrint() || c == 0)
				tmp += '#';
			else
				tmp += c;
		}

		Out(SYS_DHT | LOG_DEBUG) << tmp << endl;
	}
#endif

	RPCCall* RPCServer::doCall(RPCMsg::Ptr msg)
	{
		RPCCall* c = d->doCall(msg);
		if (c)
			connect(c, &RPCCall::timeout, this, &RPCServer::callTimeout);
		
		return c;
	}

	void RPCServer::sendMsg(RPCMsg::Ptr msg)
	{
		d->sendMsg(msg);
	}

	void RPCServer::sendMsg(const dht::RPCMsg& msg)
	{
		QByteArray data;
		msg.encode(data);
		d->send(msg.getDestination(), data);
	}
	
	void RPCServer::callTimeout(RPCCall* call)
	{
		d->timedOut(call->getRequest()->getMTID());
	}

	void RPCServer::ping(const dht::Key & our_id, const net::Address & addr)
	{
		RPCMsg::Ptr pr(new PingReq(our_id));
		pr->setOrigin(addr);
		doCall(pr);
	}

	Uint32 RPCServer::getNumActiveRPCCalls() const
	{
		return d->calls.count();
	}
}

