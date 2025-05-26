#ifndef QT_GUI_LIB
#define QT_GUI_LIB
#endif

#include <ctime>

#include <QEventLoop>
#include <QLocale>
#include <QRandomGenerator64>
#include <QTest>

#include <datachecker/multidatachecker.h>
#include <datachecker/singledatachecker.h>
#include <testlib/dummytorrentcreator.h>
#include <torrent/torrentcontrol.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

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
        QLocale::setDefault(QLocale(u"main"_s));
        QVERIFY(bt::InitLibKTorrent());
        bt::InitLog(u"datacheckertest.log"_s, false, true);
    }

    void testSingleFile()
    {
        Out(SYS_GEN | LOG_DEBUG) << "testSingleFile" << endl;
        DummyTorrentCreator creator;
        bt::TorrentControl tc;
        QVERIFY(creator.createSingleFileTorrent(TEST_FILE_SIZE, u"test.avi"_s));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try {
            tc.init(nullptr, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0"_L1, creator.tempPath() + "data/"_L1);
            tc.createFiles();
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        SingleDataChecker dc(0, tc.getStats().total_chunks);
        try {
            QString dnd = tc.getTorDir() + "dnd"_L1 + bt::DirSeparator();
            dc.check(tc.getStats().output_path, tc.getTorrent(), dnd, tc.downloadedChunksBitSet());
            QVERIFY(dc.getResult().allOn());
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Datacheck failed: " << err.toString() << endl;
            QFAIL("Torrent load failure");
        }
    }

    void testMultiFile()
    {
        Out(SYS_GEN | LOG_DEBUG) << "testMultiFile" << endl;
        QMap<QString, bt::Uint64> files;

        files[u"aaa.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"bbb.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"ccc.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);

        DummyTorrentCreator creator;
        bt::TorrentControl tc;
        QVERIFY(creator.createMultiFileTorrent(files, u"movies"_s));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try {
            tc.init(nullptr, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0"_L1, creator.tempPath() + "data/"_L1);
            tc.createFiles();
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        MultiDataChecker dc(0, tc.getStats().total_chunks);
        try {
            QString dnd = tc.getTorDir() + "dnd"_L1 + bt::DirSeparator();
            dc.check(tc.getStats().output_path, tc.getTorrent(), dnd, tc.downloadedChunksBitSet());
            QVERIFY(dc.getResult().allOn());
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Datacheck failed: " << err.toString() << endl;
            QFAIL("Torrent check failure");
        }
    }

    void testPartial()
    {
        QMap<QString, bt::Uint64> files;

        files[u"aaa.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"bbb.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"ccc.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"ddd.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);

        DummyTorrentCreator creator;
        bt::TorrentControl tc;
        QVERIFY(creator.createMultiFileTorrent(files, u"movies"_s));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try {
            tc.init(nullptr, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0"_L1, creator.tempPath() + "data/"_L1);
            tc.createFiles();
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        for (Uint32 file = 0; file < tc.getNumFiles(); file++) {
            const bt::TorrentFileInterface &fi = tc.getTorrentFile(file);
            MultiDataChecker dc(fi.getFirstChunk(), fi.getLastChunk());
            try {
                QString dnd = tc.getTorDir() + "dnd"_L1 + bt::DirSeparator();
                dc.check(tc.getStats().output_path, tc.getTorrent(), dnd, tc.downloadedChunksBitSet());
                for (Uint32 i = 0; i < tc.getStats().total_chunks; i++)
                    QVERIFY(dc.getResult().get(i) == (i >= fi.getFirstChunk() && i <= fi.getLastChunk()));
            } catch (bt::Error &err) {
                Out(SYS_GEN | LOG_DEBUG) << "Datacheck failed: " << err.toString() << endl;
                QFAIL("Torrent check failure");
            }
        }
    }

private:
};

QTEST_MAIN(DataCheckerTest)

#include "datacheckertest.moc"
