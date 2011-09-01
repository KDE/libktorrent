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
#include <net/portlist.h>
#include <util/log.h>
#include <util/error.h>
#include <torrent/globals.h>
#include <bcodec/bnode.h>
#include <bcodec/bdecoder.h>
#include <bcodec/bencoder.h>
#include <util/functions.h>
#include "rpccall.h"
#include "rpcmsg.h"
#include "kbucket.h"
#include "node.h"
#include "dht.h"
#include <net/socket.h>

using namespace bt;

namespace dht
{
	
	class RPCServer::Private : public QThread
	{
	public:
		Private(RPCServer* p,DHT* dh_table,Uint16 port) 
			: p(p),dh_table(dh_table),next_mtid(0),port(port),running(false)
		{}
		
		~Private()
		{
			bt::Globals::instance().getPortList().removePort(port,net::UDP);
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
			net::Address addr(ip,port);
			net::Socket::Ptr sock(new net::Socket(false,addr.ipVersion()));
			if (!sock->bind(addr,false))
			{
				Out(SYS_DHT|LOG_IMPORTANT) << "DHT: Failed to bind to " << addr.toString() << endl;
			}
			else
			{
				Out(SYS_DHT|LOG_NOTICE) << "DHT: Bound to " << addr.toString() << endl;
				sockets.append(sock);
			}
		}
		
		virtual void run()
		{
			net::Poll poller;
			running = true;
			while (running)
			{
				foreach (net::Socket::Ptr sock,sockets)
					sock->prepare(&poller,net::Poll::INPUT);
				
				
				if (poller.poll(500) > 0)
				{
					foreach (net::Socket::Ptr sock,sockets)
					{
						if (sock->ready(&poller,net::Poll::INPUT))
							handlePacket(sock);
					}
				}
				
				poller.reset();
			}
		}
		
		void handlePacket(net::Socket::Ptr socket)
		{
			QMutexLocker lock(&msg_mutex);
			static bt::Uint8 packet_buf[2048];
			
			net::Address addr;
			int ret = socket->recvFrom(packet_buf,2048,addr);
			if (ret <= 0)
				return;
			
			
			BNode* n = 0;
			try
			{
				QByteArray data = QByteArray::fromRawData((const char*)packet_buf,ret);
				// read and decode the packet
				BDecoder bdec(data,false);
				n = bdec.decode();
				
				if (!n || n->getType() != BNode::DICT)
				{
					delete n;
					return;
				}
				
				// try to make a RPCMsg of it
				MsgBase::Ptr msg = MakeRPCMsg((BDictNode*)n,p);
				if (msg)
				{
					msg->setOrigin(addr);
					incoming.append(msg);
				}
			}
			catch (bt::Error & err)
			{
				Out(SYS_DHT|LOG_IMPORTANT) << "Error happened during parsing : " << err.toString() << endl;
			}
			delete n;
		}
		
		void stop()
		{
			running = false;
		}
		
		
		MsgBase::Ptr nextMessage()
		{
			if (incoming.isEmpty())
				return MsgBase::Ptr(0);
			
			MsgBase::Ptr msg = incoming.first();
			incoming.pop_front();
			return msg;
		}
		
		void send(const net::Address & addr,const QByteArray & msg)
		{
			foreach (net::Socket::Ptr sock,sockets)
			{
				if (sock->sendTo((const bt::Uint8*)msg.data(),msg.size(),addr) == msg.size())
					break;
			}
		}
		
		void doQueuedCalls()
		{
			while (call_queue.count() > 0 && calls.count() < 256)
			{
				RPCCall* c = call_queue.first();
				call_queue.removeFirst();
				
				while (calls.contains(next_mtid))
					next_mtid++;
				
				MsgBase::Ptr msg = c->getRequest();
				msg->setMTID(next_mtid++);
				sendMsg(msg);
				calls.insert(msg->getMTID(),c);
				c->start();
			}
		}
		
		RPCCall* doCall(MsgBase::Ptr msg)
		{
			QMutexLocker lock(&mutex);
			Uint8 start = next_mtid;
			while (calls.contains(next_mtid))
			{
				next_mtid++;
				if (next_mtid == start) // if this happens we cannot do any calls
				{
					// so queue the call
					RPCCall* c = new RPCCall(p,msg,true);
					call_queue.append(c);
					Out(SYS_DHT|LOG_NOTICE) << "Queueing RPC call, no slots available at the moment" << endl;
					return c; 
				}
			}
			
			msg->setMTID(next_mtid++);
			sendMsg(msg);
			RPCCall* c = new RPCCall(p,msg,false);
			calls.insert(msg->getMTID(),c);
			return c;
		}
		
		void sendMsg(MsgBase::Ptr msg)
		{
			QByteArray data;
			msg->encode(data);
			send(msg->getDestination(),data);
			//	PrintRawData(data);
		}
		
		void timedOut(Uint8 mtid)
		{
			QMutexLocker lock(&mutex);
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
		QList<net::Socket::Ptr> sockets;
		DHT* dh_table;
		bt::PtrMap<bt::Uint8,RPCCall> calls;
		QList<RPCCall*> call_queue;
		bt::Uint8 next_mtid;
		bt::Uint16 port;
		QMutex mutex;
		QMutex msg_mutex;
		bool running;
		QList<MsgBase::Ptr> incoming;
	};
	


	RPCServer::RPCServer(DHT* dh_table,Uint16 port,QObject *parent)
		: QObject(parent),d(new Private(this,dh_table,port))
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
		foreach (const QString & addr,ips)
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
		{
			bt::Globals::instance().getPortList().addNewPort(d->port,net::UDP,true);
			d->start(QThread::IdlePriority);
		}
	}
		
	void RPCServer::stop()
	{
		bt::Globals::instance().getPortList().removePort(d->port,net::UDP);
		if (d->running)
		{
			d->stop();
			d->wait();
		}
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
		
		Out(SYS_DHT|LOG_DEBUG) << tmp << endl;
	}
#endif

	void RPCServer::handlePackets()
	{
		// lock the thread
		QMutexLocker lock(&d->msg_mutex);
		
		MsgBase::Ptr msg(0);
		while ((msg = d->nextMessage()) != 0)
		{
			msg->apply(d->dh_table,msg);
			// erase an existing call
			if (msg->getType() == RSP_MSG && d->calls.contains(msg->getMTID()))
			{
				// delete the call, but first notify it off the response
				RPCCall* c = d->calls.find(msg->getMTID());
				c->response(msg);
				d->calls.erase(msg->getMTID());
				c->deleteLater();
				d->doQueuedCalls();
			}
		}
	}
	
	RPCCall* RPCServer::doCall(MsgBase::Ptr msg)
	{
		return d->doCall(msg);
	}
	
	void RPCServer::sendMsg(MsgBase::Ptr msg)
	{
		d->sendMsg(msg);
	}
	
	void RPCServer::sendMsg(const dht::MsgBase& msg)
	{
		QByteArray data;
		msg.encode(data);
		d->send(msg.getDestination(),data);
	}

	
	void RPCServer::timedOut(Uint8 mtid)
	{
		d->timedOut(mtid);
	}
	
	Method RPCServer::findMethod(Uint8 mtid)
	{
		QMutexLocker lock(&d->mutex);
		const RPCCall* call = d->calls.find(mtid);
		if (call)
			return call->getMsgMethod();
		else
			return dht::NONE;
	}
	
	void RPCServer::ping(const dht::Key & our_id,const net::Address & addr)
	{
		Out(SYS_DHT|LOG_NOTICE) << "DHT: pinging " << addr.toString() << endl;
		MsgBase::Ptr pr(new PingReq(our_id));
		pr->setOrigin(addr);
		doCall(pr);
	}
	
	Uint32 RPCServer::getNumActiveRPCCalls() const
	{
		return d->calls.count();
	}

	

}
#include "rpcserver.moc"
