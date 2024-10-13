#ifndef QT_GUI_LIB
#define QT_GUI_LIB
#endif

#include <ctime>

#include <QEventLoop>
#include <QLocale>
#include <QTest>

#include "testlib/dummytorrentcreator.h"
#include <diskio/chunkmanager.h>
#include <download/downloader.h>
#include <download/streamingchunkselector.h>
#include <interfaces/piecedownloader.h>
#include <torrent/torrentcontrol.h>
#include <util/bitset.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

const bt::Uint64 TEST_FILE_SIZE = 15 * 1024 * 1024;

class DummyDownloader : public PieceDownloader
{
public:
    ~DummyDownloader() override
    {
    }

    bool canAddRequest() const override
    {
        return true;
    }
    void cancel(const bt::Request &) override
    {
    }
    void cancelAll() override
    {
    }
    bool canDownloadChunk() const override
    {
        return getNumGrabbed() == 0;
    }
    void download(const bt::Request &) override
    {
    }
    void checkTimeouts() override
    {
    }
    Uint32 getDownloadRate() const override
    {
        return 0;
    }
    QString getName() const override
    {
        return "foobar";
    }
    bool isChoked() const override
    {
        return false;
    }
};

class ExtendedStreamingChunkSelector : public bt::StreamingChunkSelector
{
public:
    ExtendedStreamingChunkSelector()
    {
    }
    ~ExtendedStreamingChunkSelector() override
    {
    }

    void markDownloaded(Uint32 i)
    {
        cman->chunkDownloaded(i);
    }

    Downloader *downloader()
    {
        return downer;
    }
};

class StreamingChunkSelectorTest : public QEventLoop
{
    Q_OBJECT

public:
    StreamingChunkSelectorTest()
    {
    }

    StreamingChunkSelectorTest(QObject *parent)
        : QEventLoop(parent)
    {
    }

private Q_SLOTS:
    void initTestCase()
    {
        QLocale::setDefault(QLocale("main"));
        bt::InitLibKTorrent();
        bt::InitLog("streamingchunkselectortest.log", false, true);
    }

    void testSimple()
    {
        DummyTorrentCreator creator;
        bt::TorrentControl tc;
        QVERIFY(creator.createSingleFileTorrent(TEST_FILE_SIZE, "test.avi"));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try {
            tc.init(nullptr, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0"_L1, creator.tempPath() + "data/"_L1);
            tc.createFiles();
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        ExtendedStreamingChunkSelector *csel = new ExtendedStreamingChunkSelector();
        tc.setChunkSelector(csel);
        QVERIFY(csel != nullptr);
        csel->setSequentialRange(0, 50);

        for (Uint32 i = 0; i < 50; i++) {
            DummyDownloader dd;
            Uint32 selected = 0xFFFFFFFF;
            QVERIFY(csel->select(&dd, selected));
            Out(SYS_GEN) << "i = " << i << ", selected = " << selected << endl;
            QVERIFY(selected == i);
            csel->markDownloaded(i);
        }

        // cleanup
        tc.setChunkSelector(nullptr);
    }

    void testCriticalChunkSpread()
    {
        DummyTorrentCreator creator;
        bt::TorrentControl tc;
        QVERIFY(creator.createSingleFileTorrent(2 * TEST_FILE_SIZE, "test2.avi"));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try {
            tc.init(nullptr, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0"_L1, creator.tempPath() + "data/"_L1);
            tc.createFiles();
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        ExtendedStreamingChunkSelector *csel = new ExtendedStreamingChunkSelector();
        tc.setChunkSelector(csel);
        QVERIFY(csel != nullptr);
        Downloader *downer = csel->downloader();
        QVERIFY(downer != nullptr);

        // Check that critical chunks are spread over multiple peers
        csel->setSequentialRange(20, 60);
        DummyDownloader dd[32];
        for (Uint32 i = 0; i < 32; i++) {
            downer->addPieceDownloader(&dd[i]);
        }

        // Check the spread of the downloaders
        downer->update();
        for (Uint32 i = 20; i < csel->criticialWindowSize(); i++) {
            QVERIFY(downer->downloading(i));
            QVERIFY(downer->download(i)->getNumDownloaders() == 32 / csel->criticialWindowSize());
        }

        for (Uint32 i = 0; i < 32; i++) {
            QVERIFY(dd[i].getNumGrabbed() == 1);
        }

        for (Uint32 i = 0; i < 32; i++) {
            downer->removePieceDownloader(&dd[i]);
        }
        // cleanup
        tc.setChunkSelector(nullptr);
    }
};

QTEST_MAIN(StreamingChunkSelectorTest)

#include "streamingchunkselectortest.moc"
