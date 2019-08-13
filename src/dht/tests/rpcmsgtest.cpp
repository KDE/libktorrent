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

#include <QtTest>
#include <util/log.h>
#include <util/error.h>
#include <dht/rpcmsgfactory.h>
#include <dht/rpcmsg.h>
#include <dht/errmsg.h>
#include <bcodec/bdecoder.h>
#include <bcodec/bnode.h>

class RPCMsgTest : public QObject, public dht::RPCMethodResolver
{
	Q_OBJECT
private:
	dht::Method findMethod(const QByteArray& mtid) override
	{
		Q_UNUSED(mtid);
		return current_method;
	}
	
private Q_SLOTS:
	void initTestCase()
	{
		bt::InitLog("rpcmsgtest.log", false, true);
	}

	void cleanupTestCase()
	{
	}

	void testErrMsg()
	{
		const char* msg = "d1:eli201e23:A Generic Error Ocurrede1:t2:aa1:y1:ee";

		try
		{
			bt::BDecoder dec(QByteArray(msg), false);

			QScopedPointer<bt::BDictNode> dict(dec.decodeDict());
			dht::RPCMsg::Ptr msg = factory.build(dict.data(), 0);

			QVERIFY(msg->getType() == dht::ERR_MSG);
			dht::ErrMsg::Ptr err = msg.dynamicCast<dht::ErrMsg>();
			QVERIFY(err);
			QVERIFY(err->message() == "A Generic Error Ocurred");
			QVERIFY(err->getMTID() == QByteArray("aa"));

		}
		catch (bt::Error & e)
		{
			QFAIL(e.toString().toLocal8Bit().data());
		}
	}

	void testWrongErrMsg()
	{
		const char* msg[] = {"d1:t2:aa1:y1:ee", "d1:eli201e1:t2:aa1:y1:ee", 0};

		int idx = 0;
		while (msg[idx])
		{
			bt::BDecoder dec(QByteArray(msg[idx]), false);
			QScopedPointer<bt::BDictNode> dict(dec.decodeDict());
			try
			{
				dht::RPCMsg::Ptr msg = factory.build(dict.data(), this);
				QFAIL("No exception thrown");
			}
			catch (bt::Error & )
			{
				// OK
			}
			idx++;
		}
	}
	
	void testPing()
	{
		const char* msg[] = {"d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe", "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re", 0};
		current_method = dht::PING;
		
		int idx = 0;
		while (msg[idx])
		{
			bt::BDecoder dec(QByteArray(msg[idx]), false);
			QScopedPointer<bt::BDictNode> dict(dec.decodeDict());
			try
			{
				dht::RPCMsg::Ptr msg = factory.build(dict.data(), this);
				QVERIFY(msg);
				QVERIFY(msg->getMTID() == QByteArray("aa"));
				QVERIFY(msg->getMethod() == dht::PING);
			}
			catch (bt::Error & e)
			{
				QFAIL(e.toString().toLocal8Bit().data());
			}
			idx++;
		}
	}
	
	void testFindNode()
	{
		const char* msg[] = {
			"d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe", 
			"d1:rd2:id20:0123456789abcdefghij5:nodes9:def456...e1:t2:aa1:y1:re", 
			0
		};
		
		current_method = dht::FIND_NODE;
		
		int idx = 0;
		while (msg[idx])
		{
			bt::BDecoder dec(QByteArray(msg[idx]), false);
			QScopedPointer<bt::BDictNode> dict(dec.decodeDict());
			try
			{
				dht::RPCMsg::Ptr msg = factory.build(dict.data(), this);
				QVERIFY(msg);
				QVERIFY(msg->getMTID() == QByteArray("aa"));
				QVERIFY(msg->getMethod() == dht::FIND_NODE);
			}
			catch (bt::Error & e)
			{
				QFAIL(e.toString().toLocal8Bit().data());
			}
			idx++;
		}
	}
	
	void testGetPeers()
	{
		const char* msg[] = {
			"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe",
			"d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:axje.u6:idhtnmee1:t2:aa1:y1:re",
			"d1:rd2:id20:abcdefghij01234567895:nodes9:def456...5:token8:aoeusnthe1:t2:aa1:y1:re",
			0
		};
		current_method = dht::GET_PEERS;
		
		int idx = 0;
		while (msg[idx])
		{
			bt::BDecoder dec(QByteArray(msg[idx]), false);
			QScopedPointer<bt::BDictNode> dict(dec.decodeDict());
			try
			{
				dht::RPCMsg::Ptr msg = factory.build(dict.data(), this);
				QVERIFY(msg);
				QVERIFY(msg->getMTID() == QByteArray("aa"));
				QVERIFY(msg->getMethod() == dht::GET_PEERS);
			}
			catch (bt::Error & e)
			{
				QFAIL(e.toString().toLocal8Bit().data());
			}
			idx++;
		}
	}
	
	void testAnnounce()
	{
		const char* msg[] = {
			"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
			"d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re",
			0
		};
		current_method = dht::ANNOUNCE_PEER;
		int idx = 0;
		while (msg[idx])
		{
			bt::BDecoder dec(QByteArray(msg[idx]), false);
			QScopedPointer<bt::BDictNode> dict(dec.decodeDict());
			try
			{
				dht::RPCMsg::Ptr msg = factory.build(dict.data(), this);
				QVERIFY(msg);
				QVERIFY(msg->getMTID() == QByteArray("aa"));
				QVERIFY(msg->getMethod() == dht::ANNOUNCE_PEER);
			}
			catch (bt::Error & e)
			{
				QFAIL(e.toString().toLocal8Bit().data());
			}
			idx++;
		}
	}

private:
	dht::RPCMsgFactory factory;
	dht::Method current_method;
};

QTEST_MAIN(RPCMsgTest)

#include "rpcmsgtest.moc"
