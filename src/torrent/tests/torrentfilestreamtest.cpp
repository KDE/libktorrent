#include <QtTest>
#include <QObject>
#include <KGlobal>
#include <KLocale>
#include <unistd.h>
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
	}
	
	void cleanupTestCase()
	{
	}
	
	void testStreaming()
	{
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
		}
		
		stream->close();
		delete stream;
	}
	
private:
	DummyTorrentCreator creator;
	bt::TorrentControl tc;
};

QTEST_MAIN(TorrentFileStreamTest)

#include "torrentfilestreamtest.moc"