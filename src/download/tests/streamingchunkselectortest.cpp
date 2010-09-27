#define QT_GUI_LIB

#include <time.h>
#include <QtTest>
#include <KGlobal>
#include <KLocale>
#include <QEventLoop>
#include <util/log.h>
#include <util/error.h>
#include <util/bitset.h>
#include <util/functions.h>
#include <torrent/torrentcontrol.h>
#include <interfaces/piecedownloader.h>
#include <download/streamingchunkselector.h>
#include <download/downloader.h>
#include <diskio/chunkmanager.h>
#include "testlib/dummytorrentcreator.h"




using namespace bt;

const bt::Uint64 TEST_FILE_SIZE = 15*1024*1024;

class DummyDownloader : public PieceDownloader
{
public:
	virtual ~DummyDownloader() {}
	
	virtual bool canAddRequest() const {return true;}
	virtual void cancel(const bt::Request& ) {}
	virtual void cancelAll() {}
	virtual bool canDownloadChunk() const {return getNumGrabbed() == 0;}
	virtual void download(const bt::Request& ) {}
	virtual void checkTimeouts() {}
	virtual Uint32 getDownloadRate() const {return 0;}
	virtual QString getName() const {return "foobar";}
    virtual bool isChoked() const {return false;}
};

class ExtendedStreamingChunkSelector : public bt::StreamingChunkSelector
{
public:
	ExtendedStreamingChunkSelector() {}
	virtual ~ExtendedStreamingChunkSelector() {}
	
	void markDownloaded(Uint32 i)
	{
		cman->chunkDownloaded(i);
	}
	
	Downloader* downloader() {return downer;}
};

class StreamingChunkSelectorTest : public QEventLoop
{
	Q_OBJECT
	
public:
	StreamingChunkSelectorTest()
	{}
	
	StreamingChunkSelectorTest(QObject* parent) : QEventLoop(parent)
	{}
	
private slots:
	void initTestCase()
	{
		KGlobal::setLocale(new KLocale("main"));
		bt::InitLibKTorrent();
		bt::InitLog("streamingchunkselectortest.log",false,true);
		qsrand(time(0));
	}
	
	void testSimple()
	{
		DummyTorrentCreator creator;
		bt::TorrentControl tc;
		QVERIFY(creator.createSingleFileTorrent(TEST_FILE_SIZE,"test.avi"));
		
		Out(SYS_GEN|LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
		try
		{
			tc.init(0,creator.torrentPath(),creator.tempPath() + "tor0",creator.tempPath() + "data/");
			tc.createFiles();
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
			QFAIL("Torrent load failure");
		}
		
		
		ExtendedStreamingChunkSelector* csel = new ExtendedStreamingChunkSelector();
		tc.setChunkSelector(csel);
		QVERIFY(csel != 0);
		csel->setSequentialRange(0,50);
		
		for (Uint32 i = 0;i < 50;i++)
		{
			DummyDownloader dd;
			Uint32 selected = 0xFFFFFFFF;
			QVERIFY(csel->select(&dd,selected));
			Out(SYS_GEN) << "i = " << i << ", selected = " << selected << endl;
			QVERIFY(selected == i);
			csel->markDownloaded(i);
		}
		
		// cleanup
		tc.setChunkSelector(0);
	}
	
	void testCriticalChunkSpread()
	{
		DummyTorrentCreator creator;
		bt::TorrentControl tc;
		QVERIFY(creator.createSingleFileTorrent(TEST_FILE_SIZE,"test2.avi"));
		
		Out(SYS_GEN|LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
		try
		{
			tc.init(0,creator.torrentPath(),creator.tempPath() + "tor0",creator.tempPath() + "data/");
			tc.createFiles();
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
			QFAIL("Torrent load failure");
		}
		
		ExtendedStreamingChunkSelector* csel = new ExtendedStreamingChunkSelector();
		tc.setChunkSelector(csel);
		QVERIFY(csel != 0);
		Downloader* downer = csel->downloader();
		QVERIFY(downer != 0);
		
		// Check that critical chunks are spread over multiple peers
		csel->setSequentialRange(0,50);
		DummyDownloader dd[32];
		for (Uint32 i = 0;i < 16;i++)
		{
			downer->addPieceDownloader(&dd[i]);
		}
		
		// There should now be 16 ChunkDownload's each for one of the criticial chunks
		downer->update();
		for (Uint32 i = 0;i < 16;i++)
		{
			QVERIFY(dd[i].getNumGrabbed() == 1);
			QVERIFY(downer->downloading(i));
			QVERIFY(downer->download(i)->getNumDownloaders() == 1);
		}
		
		// Add the second batch
		for (Uint32 i = 16;i < 32;i++)
		{
			downer->addPieceDownloader(&dd[i]);
		}
		
		downer->update();
		
		for (Uint32 i = 0;i < 32;i++)
		{
			QVERIFY(dd[i].getNumGrabbed() == 1);
			if (i < 16)
			{
				QVERIFY(downer->downloading(i));
				// There should now be two downloaders per chunk
				QVERIFY(downer->download(i)->getNumDownloaders() == 2);
			}
		}
		
		for (Uint32 i = 0;i < 32;i++)
		{
			downer->removePieceDownloader(&dd[i]);
		}
		// cleanup
		tc.setChunkSelector(0);
	}
};

QTEST_MAIN(StreamingChunkSelectorTest)

#include "streamingchunkselectortest.moc"