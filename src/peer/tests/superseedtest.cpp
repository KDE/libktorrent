/***************************************************************************
*   Copyright (C) 2010 by Joris Guisson                                   *
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
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
***************************************************************************/

#include <QtTest>
#include <QObject>
#include <util/log.h>
#include <peer/superseeder.h>
#include <interfaces/peerinterface.h>
#include <peer/chunkcounter.h>

using namespace bt;

#define NUM_CHUNKS 10

class DummyPeer : public PeerInterface
{
public:
	DummyPeer(const char* n) : PeerInterface(PeerID(),NUM_CHUNKS)
	{
		QString name = n;
		if (name.size() < 20)
			name = name.leftJustified(20,'x');
		peer_id = PeerID(name.toAscii());
	}
	
	virtual ~DummyPeer()
	{}
	
	void reset()
	{
		pieces.setAll(false);
	}
	
	void have(Uint32 chunk)
	{
		pieces.set(chunk,true);
	}
	
	virtual void kill() {}
	virtual Uint32 averageDownloadSpeed() const {return 0;}
};

class SuperSeedTest : public QObject, public SuperSeederClient
{
	Q_OBJECT
public:
	SuperSeedTest(QObject* parent = 0) : QObject(parent),a("a"),b("b"),c("c")
	{}
	
    virtual void allowChunk(PeerInterface* peer, Uint32 chunk)
    {
		Out(SYS_GEN|LOG_DEBUG) << "Allow called on " << peer->getPeerID().toString() << " " << chunk << endl;
		allow_called = true;
		allowed_chunks.append(chunk);
		target = peer;
	}
	
	bool allowCalled(PeerInterface* peer) 
	{
		bool ret = allow_called;
		allow_called = false;
		return ret && target == peer;
	}
	
private Q_SLOTS:
	void initTestCase()
	{
		bt::InitLog("superseedtest.log");
		allow_called = false;
	}
	
	void cleanupTestCase()
	{
	}
	
	void testSuperSeeding()
	{
		ChunkCounter cnt(NUM_CHUNKS);
		SuperSeeder ss(&cnt,this);
		a.reset();
		b.reset();
		c.reset();
		
		// First test, all tree should get a chunk
		ss.peerAdded(&a);
		QVERIFY(allowCalled(&a));
		QVERIFY(allowed_chunks.count() == 1);
		ss.peerAdded(&b);
		QVERIFY(allowCalled(&b));
		QVERIFY(allowed_chunks.count() == 2);
		ss.peerAdded(&c);
		QVERIFY(allowCalled(&c));
		QVERIFY(allowed_chunks.count() == 3);
		
		DummyPeer* uploader = &a;
		DummyPeer* downloader = &b;
		DummyPeer* next = &c;
		// Simulate normal superseeding operation
		for (int i = 0;i < 4;i++)
		{
			int nallowed = allowed_chunks.count();
			// Now simulate b downloaded the first chunk from a
			Uint32 chunk = allowed_chunks.at(i);
			uploader->have(chunk);
			downloader->have(chunk);
			ss.have(downloader,chunk);
			QVERIFY(allowCalled(uploader)); // allow should be called again on the uploader
			QVERIFY(allowed_chunks.count() == nallowed + 1);
			
			// Cycle through peers
			DummyPeer* n = uploader;
			uploader = downloader;
			downloader = next;
			next = n;
		}
	}

private:
	DummyPeer a,b,c;
	bool allow_called;
	QList<Uint32> allowed_chunks;
	PeerInterface* target;
};

QTEST_MAIN(SuperSeedTest)

#include "superseedtest.moc"