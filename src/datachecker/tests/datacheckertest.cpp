#include <unistd.h>
#include <time.h>

#define QT_GUI_LIB
#include <QtTest>
#include <QEventLoop>
#include <KLocale>
#include <KGlobal>
#include <util/log.h>
#include <util/error.h>
#include <util/functions.h>
#include <torrent/torrentcontrol.h>
#include <datachecker/singledatachecker.h>
#include <datachecker/multidatachecker.h>
#include <testlib/dummytorrentcreator.h>


using namespace bt;

const bt::Uint64 TEST_FILE_SIZE = 15*1024*1024;

bt::Uint64 RandomSize(bt::Uint64 min_size,bt::Uint64 max_size)
{
	bt::Uint64 r = max_size - min_size;
	return min_size + qrand() % r;
}

class DataCheckerTest : public QEventLoop
{
	Q_OBJECT
	
public:
	
private slots:
	void initTestCase()
	{
		KGlobal::setLocale(new KLocale("main"));
		bt::InitLibKTorrent();
		bt::InitLog("datacheckertest.log",false,true);
		qsrand(time(0));
	}
	
	void testSingleFile()
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
		
		SingleDataChecker dc;
		try
		{
			QString dnd = tc.getTorDir() + "dnd" + bt::DirSeparator();
			dc.check(tc.getStats().output_path,tc.getTorrent(),dnd,tc.downloadedChunksBitSet());
			QVERIFY(dc.getResult().allOn());
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Datacheck failed: " << err.toString() << endl;
			QFAIL("Torrent load failure");
		}
	}
	
	void testMultiFile()
	{
		QMap<QString,bt::Uint64> files;
		
		files["aaa.avi"] = RandomSize(TEST_FILE_SIZE / 2,TEST_FILE_SIZE);
		files["bbb.avi"] = RandomSize(TEST_FILE_SIZE / 2,TEST_FILE_SIZE);
		files["ccc.avi"] = RandomSize(TEST_FILE_SIZE / 2,TEST_FILE_SIZE);
		
		DummyTorrentCreator creator;
		bt::TorrentControl tc;
		QVERIFY(creator.createMultiFileTorrent(files,"movies"));
		
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
		
		MultiDataChecker dc;
		try
		{
			QString dnd = tc.getTorDir() + "dnd" + bt::DirSeparator();
			dc.check(tc.getStats().output_path,tc.getTorrent(),dnd,tc.downloadedChunksBitSet());
			QVERIFY(dc.getResult().allOn());
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Datacheck failed: " << err.toString() << endl;
			QFAIL("Torrent check failure");
		}
	}
	
private:
	
	
};

QTEST_MAIN(DataCheckerTest)

#include "datacheckertest.moc"

