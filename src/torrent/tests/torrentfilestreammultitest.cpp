#define QT_GUI_LIB

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




using namespace bt;

const bt::Uint32 TEST_FILE_SIZE = 5*1024*1024;

bt::Uint64 RandomSize(bt::Uint64 min_size,bt::Uint64 max_size)
{
	bt::Uint64 r = max_size - min_size;
	return min_size + qrand() % r;
}

class TorrentFileStreamMultiTest : public QEventLoop, public bt::QueueManagerInterface
{
	Q_OBJECT
public:
	TorrentFileStreamMultiTest(QObject* parent = 0) : QEventLoop(parent)
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
	
	
private slots:
	void initTestCase()
	{
		KGlobal::setLocale(new KLocale("main"));
		bt::InitLog("torrentfilestreammultitest.log",false,false);
		
		files["aaa.avi"] = RandomSize(TEST_FILE_SIZE / 2,TEST_FILE_SIZE);
		files["bbb.avi"] = RandomSize(TEST_FILE_SIZE / 2,TEST_FILE_SIZE);
		files["ccc.avi"] = RandomSize(TEST_FILE_SIZE / 2,TEST_FILE_SIZE);
		
		QVERIFY(creator.createMultiFileTorrent(files,"movies"));
		
		Out(SYS_GEN|LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
		try
		{
			tc.init(this,creator.torrentPath(),creator.tempPath() + "tor0",creator.tempPath() + "data/");
			tc.createFiles();
			QVERIFY(tc.hasExistingFiles());
			tc.startDataCheck(false);
			do
			{
				processEvents(AllEvents,1000);
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
		// Simple test read each file and check if they are the same on disk
		for (bt::Uint32 i = 0;i < tc.getNumFiles();i++)
		{
			bt::TorrentFileStream::Ptr stream = tc.createTorrentFileStream(i,false,this);
			QVERIFY(stream);
			QVERIFY(!stream->open(QIODevice::ReadWrite));
			QVERIFY(stream->open(QIODevice::ReadOnly));
			QVERIFY(stream->size() == tc.getTorrentFile(i).getSize());
		
			QByteArray tmp(stream->size(),0);
			bt::Uint64 written = 0;
			bt::Uint32 idx = 0;
			while (written < stream->size())
			{
				qint64 ret = stream->read(tmp.data(),stream->size());
				QVERIFY(ret == stream->size());
				written += ret;
				
				// Load the file and check if the data is the same
				QFile fptr(stream->path());
				QVERIFY(fptr.open(QIODevice::ReadOnly));
				QByteArray tmp2(stream->size(),0);
				QVERIFY(fptr.read(tmp2.data(),stream->size()) == stream->size());
				QVERIFY(tmp == tmp2);
			}
			
			stream->close();
		}
		Out(SYS_GEN|LOG_DEBUG) << "End: testSimple() " << endl;
	}
	
	void testSeek()
	{
		Out(SYS_GEN|LOG_DEBUG) << "Begin: testSeek() " << endl;
		
		// In each file take a couple of random samples and compare them with the actual file
		for (bt::Uint32 i = 0;i < tc.getNumFiles();i++)
		{
			bt::TorrentFileStream::Ptr stream = tc.createTorrentFileStream(i,false,this);
			QVERIFY(stream);
			QVERIFY(!stream->open(QIODevice::ReadWrite));
			QVERIFY(stream->open(QIODevice::ReadOnly));
			QVERIFY(stream->size() == tc.getTorrentFile(i).getSize());
		
			QFile fptr(stream->path());
			QVERIFY(fptr.open(QIODevice::ReadOnly));
			
			for (bt::Uint32 j = 0;j < 10;j++)
			{
				qint64 off = qrand() % (stream->size() - 256);
				QVERIFY(stream->seek(off));
				QVERIFY(fptr.seek(off));
				
				QByteArray sdata(256,0);
				QVERIFY(stream->read(sdata.data(),256) == 256);
				
				QByteArray fdata(256,0);
				QVERIFY(fptr.read(fdata.data(),256) == 256);
				QVERIFY(sdata == fdata);
			}
			stream->close();
		}
		
		Out(SYS_GEN|LOG_DEBUG) << "End: testSeek() " << endl;
	}
	
private:
	DummyTorrentCreator creator;
	bt::TorrentControl tc;
	QMap<QString,bt::Uint64> files;
};

QTEST_MAIN(TorrentFileStreamMultiTest)

#include "torrentfilestreammultitest.moc"