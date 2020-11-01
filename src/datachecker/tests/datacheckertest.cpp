#ifndef QT_GUI_LIB
#define QT_GUI_LIB
#endif

#include <unistd.h>
#include <time.h>

#include <QEventLoop>
#include <QLocale>
#include <QRandomGenerator64>
#include <QtTest>

#include <util/log.h>
#include <util/error.h>
#include <util/functions.h>
#include <util/fileops.h>
#include <torrent/torrentcontrol.h>
#include <datachecker/singledatachecker.h>
#include <datachecker/multidatachecker.h>
#include <testlib/dummytorrentcreator.h>

#include <boost/concept_check.hpp>


using namespace bt;

const bt::Uint64 TEST_FILE_SIZE = 15 * 1024 * 1024;

bt::Uint64 RandomSize(bt::Uint64 min_size, bt::Uint64 max_size)
{
    bt::Uint64 r = max_size - min_size;
    return min_size + QRandomGenerator64::global()->generate() % r;
}

class DataCheckerTest : public QEventLoop
{
    Q_OBJECT

public:

private Q_SLOTS:
    void initTestCase()
    {
        QLocale::setDefault(QLocale("main"));
        bt::InitLibKTorrent();
        bt::InitLog("datacheckertest.log", false, true);
    }

    void testSingleFile()
    {
        Out(SYS_GEN | LOG_DEBUG) << "testSingleFile" << endl;
        DummyTorrentCreator creator;
        bt::TorrentControl tc;
        QVERIFY(creator.createSingleFileTorrent(TEST_FILE_SIZE, "test.avi"));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try
        {
            tc.init(0, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0", creator.tempPath() + "data/");
            tc.createFiles();
        }
        catch (bt::Error& err)
        {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        SingleDataChecker dc(0, tc.getStats().total_chunks);
        try
        {
            QString dnd = tc.getTorDir() + "dnd" + bt::DirSeparator();
            dc.check(tc.getStats().output_path, tc.getTorrent(), dnd, tc.downloadedChunksBitSet());
            QVERIFY(dc.getResult().allOn());
        }
        catch (bt::Error& err)
        {
            Out(SYS_GEN | LOG_DEBUG) << "Datacheck failed: " << err.toString() << endl;
            QFAIL("Torrent load failure");
        }
    }

    void testMultiFile()
    {
        Out(SYS_GEN | LOG_DEBUG) << "testMultiFile" << endl;
        QMap<QString, bt::Uint64> files;

        files["aaa.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["bbb.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["ccc.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);

        DummyTorrentCreator creator;
        bt::TorrentControl tc;
        QVERIFY(creator.createMultiFileTorrent(files, "movies"));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try
        {
            tc.init(0, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0", creator.tempPath() + "data/");
            tc.createFiles();
        }
        catch (bt::Error& err)
        {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        MultiDataChecker dc(0, tc.getStats().total_chunks);
        try
        {
            QString dnd = tc.getTorDir() + "dnd" + bt::DirSeparator();
            dc.check(tc.getStats().output_path, tc.getTorrent(), dnd, tc.downloadedChunksBitSet());
            QVERIFY(dc.getResult().allOn());
        }
        catch (bt::Error& err)
        {
            Out(SYS_GEN | LOG_DEBUG) << "Datacheck failed: " << err.toString() << endl;
            QFAIL("Torrent check failure");
        }
    }

    void testPartial()
    {
        QMap<QString, bt::Uint64> files;

        files["aaa.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["bbb.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["ccc.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["ddd.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);

        DummyTorrentCreator creator;
        bt::TorrentControl tc;
        QVERIFY(creator.createMultiFileTorrent(files, "movies"));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try
        {
            tc.init(0, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0", creator.tempPath() + "data/");
            tc.createFiles();
        }
        catch (bt::Error& err)
        {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        for (Uint32 file = 0; file < tc.getNumFiles(); file++)
        {
            const bt::TorrentFileInterface& fi = tc.getTorrentFile(file);
            MultiDataChecker dc(fi.getFirstChunk(), fi.getLastChunk());
            try
            {
                QString dnd = tc.getTorDir() + "dnd" + bt::DirSeparator();
                dc.check(tc.getStats().output_path, tc.getTorrent(), dnd, tc.downloadedChunksBitSet());
                for (Uint32 i = 0; i < tc.getStats().total_chunks; i++)
                    QVERIFY(dc.getResult().get(i) == (i >= fi.getFirstChunk() && i <= fi.getLastChunk()));
            }
            catch (bt::Error& err)
            {
                Out(SYS_GEN | LOG_DEBUG) << "Datacheck failed: " << err.toString() << endl;
                QFAIL("Torrent check failure");
            }
        }
    }

private:


};

QTEST_MAIN(DataCheckerTest)

#include "datacheckertest.moc"

