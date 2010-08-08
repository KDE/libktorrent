#include <QtTest>
#include <QObject>
#include <KGlobal>
#include <KLocale>
#include <unistd.h>
#include <time.h>
#include <util/log.h>
#include <util/error.h>
#include <util/sha1hashgen.h>
#include <testlib/dummytorrentcreator.h>
#include <torrent/torrentcontrol.h>
#include <torrent/torrentfilestream.h>
#include <interfaces/queuemanagerinterface.h>
#include <datachecker/datacheckerlistener.h>




using namespace bt;

const bt::Uint32 TEST_FILE_SIZE = 5*1024*1024;

class TorrentFileStreamTest : public QEventLoop, public bt::QueueManagerInterface,public bt::DataCheckerListener
{
	Q_OBJECT
public:
	TorrentFileStreamTest(QObject* parent = 0) : QEventLoop(parent),bt::DataCheckerListener(false)
	{
	}
	
	virtual bool alreadyLoaded(const bt::SHA1Hash& ih) const
	{
		Q_UNUSED(ih);
		return false;
	}
	
	virtual void mergeAnnounceList(const bt::SHA1Hash& ih, const bt::TrackerTier* trk)
	{
		Q_UNUSED(ih);
		Q_UNUSED(trk);
	}
	
	virtual void error(const QString& err)
	{
		Out(SYS_GEN|LOG_DEBUG) << "DataCheck error: " << err << endl;
	}
	
	virtual void finished()
	{
		Out(SYS_GEN|LOG_DEBUG) << "DataCheck finished " << endl;
	}
	
	virtual void progress(Uint32 num, Uint32 total)
	{
		Out(SYS_GEN|LOG_DEBUG) << "DataCheck progress " << num << "/" << total << endl;
	}
	
	virtual void status(Uint32 num_failed, Uint32 num_found, Uint32 num_downloaded, Uint32 num_not_downloaded)
	{
		Q_UNUSED(num_failed);
		Q_UNUSED(num_found);
		Q_UNUSED(num_downloaded);
		Q_UNUSED(num_not_downloaded);
	}
	
private slots:
	void initTestCase()
	{
		KGlobal::setLocale(new KLocale("main"));
		bt::InitLog("torrentfilestreamtest.log",false,false);
		QVERIFY(creator.createSingleFileTorrent(TEST_FILE_SIZE,"test.avi"));

		Out(SYS_GEN|LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
		try
		{
			tc.init(this,creator.torrentPath(),creator.tempPath() + "tor0",creator.tempPath() + "data/");
			tc.createFiles();
			QVERIFY(tc.hasExistingFiles());
			tc.startDataCheck(this);
			do
			{
				sleep(1);
				processEvents();
			}
			while (tc.getStats().status == bt::CHECKING_DATA);
			QVERIFY(tc.getStats().completed);
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
			QFAIL("Torrent load failure");
		}
		
		qsrand(time(0));
	}
	
	void cleanupTestCase()
	{
	}
	
	void testSimple()
	{
		Out(SYS_GEN|LOG_DEBUG) << "Begin: testSimple() " << endl;
		bt::TorrentFileStream* stream = tc.createTorrentFileStream(0,this);
		QVERIFY(stream);
		QVERIFY(!stream->open(QIODevice::ReadWrite));
		QVERIFY(stream->open(QIODevice::ReadOnly));
		// Simple test read per chunk and verify if it works
		QByteArray tmp(tc.getStats().chunk_size,0);
		bt::Uint64 written = 0;
		bt::Uint32 idx = 0;
		while (written < TEST_FILE_SIZE)
		{
			qint64 ret = stream->read(tmp.data(),tc.getStats().chunk_size);
			QVERIFY(ret == tc.getStats().chunk_size);
			written += tc.getStats().chunk_size;
			
			// Verify the hash
			bt::SHA1Hash hash = bt::SHA1Hash::generate((const bt::Uint8*)tmp.data(),tmp.size());
			QVERIFY(hash == tc.getTorrent().getHash(idx));
			idx++;
		}
		
		stream->close();
		delete stream;
		Out(SYS_GEN|LOG_DEBUG) << "End: testSimple() " << endl;
	}
	
	void testSeek()
	{
		Out(SYS_GEN|LOG_DEBUG) << "Begin: testSeek() " << endl;
		bt::TorrentFileStream* stream = tc.createTorrentFileStream(0,this);
		QVERIFY(stream);
		QVERIFY(!stream->open(QIODevice::ReadWrite));
		QVERIFY(stream->open(QIODevice::ReadOnly));
		// Simple test read per chunk and seek somewhat
		bt::Uint32 chunk_size = tc.getStats().chunk_size;
		QByteArray tmp(chunk_size,0);
		bt::Uint64 written = 0;
		bt::Uint32 idx = 0;
		while (written < TEST_FILE_SIZE)
		{
			// Chunk size of last chunk can be smaller
			if (idx == tc.getStats().total_chunks - 1)
			{
				chunk_size = tc.getStats().chunk_size % tc.getStats().total_bytes;
				if (chunk_size == 0)
					chunk_size = tc.getStats().chunk_size;
			}
			
			// Lets read in two times, first at the back and then at the front
			qint64 split = qrand() % chunk_size;
			while (split == 0)
				split = qrand() % chunk_size;
			
			QVERIFY(stream->seek(idx * tc.getStats().chunk_size + split));
			qint64 ret = stream->read(tmp.data() + split,chunk_size - split);
			QVERIFY(ret == (chunk_size - split));
			written += ret;
			
			QVERIFY(stream->seek(idx * tc.getStats().chunk_size));
			ret = stream->read(tmp.data(),split);
			QVERIFY(ret == split);
			written += ret;
			
			// Verify the hash
			bt::SHA1Hash hash = bt::SHA1Hash::generate((const bt::Uint8*)tmp.data(),tmp.size());
			QVERIFY(hash == tc.getTorrent().getHash(idx));
			idx++;
		}
		
		stream->close();
		delete stream;
		
		Out(SYS_GEN|LOG_DEBUG) << "End: testSeek() " << endl;
	}
	
	void testRandomAccess()
	{
		Out(SYS_GEN|LOG_DEBUG) << "Begin: testRandomAccess() " << endl;
		bt::TorrentFileStream* stream = tc.createTorrentFileStream(0,this);
		QVERIFY(stream);
		QVERIFY(!stream->open(QIODevice::ReadWrite));
		QVERIFY(stream->open(QIODevice::ReadOnly));
		
		// Read an area of around 5 chunks in at a random location
		// And verify the 3 middle chunks
		bt::Uint64 range_size = tc.getStats().chunk_size * 5;
		qint64 off = ((qrand() % 100) / 100.0) * (tc.getStats().total_bytes - range_size);
		QVERIFY(stream->seek(off));
		
		Out(SYS_GEN|LOG_DEBUG) << "Reading random range" << endl;
		Out(SYS_GEN|LOG_DEBUG) << "range_size = " << range_size << endl;
		Out(SYS_GEN|LOG_DEBUG) << "off = " << off << endl;
		
		QByteArray range(range_size,0);
		qint64 bytes_read = 0;
		while (bytes_read < range_size)
		{
			qint64 ret = stream->read(range.data() + bytes_read,range_size - bytes_read);
			Out(SYS_GEN|LOG_DEBUG) << "ret = " << ret << endl;
			Out(SYS_GEN|LOG_DEBUG) << "read = " << bytes_read << endl;
			QVERIFY(ret > 0);
			bytes_read += ret;
		}
		
		{
			QFile fptr(stream->path());
			QVERIFY(fptr.open(QIODevice::ReadOnly));
			QByteArray tmp(range_size,0);
			fptr.seek(off);
			QVERIFY(fptr.read(tmp.data(),range_size) == range_size);
			QVERIFY(tmp == range);
		}
		
		QVERIFY(bytes_read == range_size);
		
		// Calculate the offset of the first chunk in range
		qint64 chunk_idx = off / tc.getStats().chunk_size;
		if (off % tc.getStats().chunk_size != 0)
			chunk_idx++;
		qint64 chunk_off = chunk_idx * tc.getStats().chunk_size - off;
		
		Out(SYS_GEN|LOG_DEBUG) << "chunk_idx = " << chunk_idx << endl;
		Out(SYS_GEN|LOG_DEBUG) << "chunk_off = " << chunk_off << endl;
		
		
		
		// Verify the hashes
		for (int i = 0;i < 3;i++)
		{
			bt::SHA1Hash hash = bt::SHA1Hash::generate((const bt::Uint8*)range.data() + chunk_off,tc.getStats().chunk_size);
			
			Out(SYS_GEN|LOG_DEBUG) << "chash = " << hash.toString() << endl;
			Out(SYS_GEN|LOG_DEBUG) << "whash = " << tc.getTorrent().getHash(chunk_idx).toString() << endl;
			QVERIFY(hash == tc.getTorrent().getHash(chunk_idx));
			chunk_idx++;
			chunk_off += tc.getStats().chunk_size;
		}
		
		stream->close();
		delete stream;
		
		Out(SYS_GEN|LOG_DEBUG) << "End: testRandomAccess() " << endl;
	}
	
	void testMultiSeek()
	{
		Out(SYS_GEN|LOG_DEBUG) << "Begin: testMultiSeek() " << endl;
		bt::TorrentFileStream* stream = tc.createTorrentFileStream(0,this);
		QVERIFY(stream);
		QVERIFY(!stream->open(QIODevice::ReadWrite));
		QVERIFY(stream->open(QIODevice::ReadOnly));
		
		QFile fptr(stream->path());
		QVERIFY(fptr.open(QIODevice::ReadOnly));
		for (int i = 0;i < 20;i++)
		{
			qint64 off = qrand() % (TEST_FILE_SIZE - 100);
			// Seek to a random location
			QVERIFY(stream->seek(off));
			QByteArray tmp(100,0);
			QVERIFY(stream->read(tmp.data(),100) == 100);
		
			// Verify those
			QByteArray tmp2(100,0);
			fptr.seek(off);
			QVERIFY(fptr.read(tmp2.data(),100) == 100);
			QVERIFY(tmp == tmp2);
		}
		
		Out(SYS_GEN|LOG_DEBUG) << "End: testMultiSeek() " << endl;
	}
	
private:
	DummyTorrentCreator creator;
	bt::TorrentControl tc;
};

QTEST_MAIN(TorrentFileStreamTest)

#include "torrentfilestreamtest.moc"