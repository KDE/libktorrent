#ifndef QT_GUI_LIB
#define QT_GUI_LIB
#endif

#include <QLocale>
#include <QObject>
#include <QRandomGenerator>
#include <QTest>

#include <ctime>
#include <interfaces/queuemanagerinterface.h>
#include <testlib/dummytorrentcreator.h>
#include <torrent/torrentcontrol.h>
#include <torrent/torrentfilestream.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/log.h>
#include <util/sha1hashgen.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

const bt::Uint32 TEST_FILE_SIZE = 5 * 1024 * 1024;

class TorrentFileStreamTest : public QEventLoop, public bt::QueueManagerInterface
{
    Q_OBJECT
public:
    TorrentFileStreamTest(QObject *parent = nullptr)
        : QEventLoop(parent)
    {
    }

    bool alreadyLoaded(const bt::SHA1Hash &ih) const override
    {
        Q_UNUSED(ih);
        return false;
    }

    void mergeAnnounceList(const bt::SHA1Hash &ih, const bt::TrackerTier *trk) override
    {
        Q_UNUSED(ih);
        Q_UNUSED(trk);
    }

private Q_SLOTS:
    void initTestCase()
    {
        QLocale::setDefault(QLocale(u"main"_s));
        bt::InitLog(u"torrentfilestreamtest.log"_s, false, false);
        QVERIFY(creator.createSingleFileTorrent(TEST_FILE_SIZE, u"test.avi"_s));
        QVERIFY(creator2.createSingleFileTorrent(TEST_FILE_SIZE, u"test2.avi"_s));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator2.torrentPath() << endl;
        try {
            tc.init(this, bt::LoadFile(creator.torrentPath()), creator.tempPath() + "tor0"_L1, creator.tempPath() + "data/"_L1);
            tc.createFiles();
            QVERIFY(tc.hasExistingFiles());
            tc.startDataCheck(false, 0, tc.getStats().total_chunks);
            do {
                processEvents(AllEvents, 1000);
            } while (tc.getStats().status == bt::CHECKING_DATA);
            QVERIFY(tc.getStats().completed);

            incomplete_tc.init(this, bt::LoadFile(creator2.torrentPath()), creator2.tempPath() + "tor0"_L1, creator2.tempPath() + "data/"_L1);
            incomplete_tc.createFiles();
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }
    }

    void cleanupTestCase()
    {
    }

    void testSimple()
    {
        Out(SYS_GEN | LOG_DEBUG) << "Begin: testSimple() " << endl;
        bt::TorrentFileStream::Ptr stream = tc.createTorrentFileStream(0, false, this);
        QVERIFY(stream);
        QVERIFY(!stream->open(QIODevice::ReadWrite));
        QVERIFY(stream->open(QIODevice::ReadOnly));
        // Simple test read per chunk and verify if it works
        QByteArray tmp(tc.getStats().chunk_size, 0);
        bt::Uint64 written = 0;
        bt::Uint32 idx = 0;
        while (written < TEST_FILE_SIZE) {
            qint64 ret = stream->read(tmp.data(), tc.getStats().chunk_size);
            QVERIFY(ret == tc.getStats().chunk_size);
            written += tc.getStats().chunk_size;

            // Verify the hash
            bt::SHA1Hash hash = bt::SHA1Hash::generate((const bt::Uint8 *)tmp.data(), tmp.size());
            QVERIFY(hash == tc.getTorrent().getHash(idx));
            idx++;
        }

        stream->close();
        Out(SYS_GEN | LOG_DEBUG) << "End: testSimple() " << endl;
    }

    void testSeek()
    {
        Out(SYS_GEN | LOG_DEBUG) << "Begin: testSeek() " << endl;
        bt::TorrentFileStream::Ptr stream = tc.createTorrentFileStream(0, false, this);
        QVERIFY(stream);
        QVERIFY(!stream->open(QIODevice::ReadWrite));
        QVERIFY(stream->open(QIODevice::ReadOnly));
        // Simple test read per chunk and seek somewhat
        bt::Uint32 chunk_size = tc.getStats().chunk_size;
        QByteArray tmp(chunk_size, 0);
        bt::Uint64 written = 0;
        bt::Uint32 idx = 0;
        while (written < TEST_FILE_SIZE) {
            // Chunk size of last chunk can be smaller
            if (idx == tc.getStats().total_chunks - 1) {
                chunk_size = tc.getStats().chunk_size % tc.getStats().total_bytes;
                if (chunk_size == 0)
                    chunk_size = tc.getStats().chunk_size;
            }

            // Lets read in two times, first at the back and then at the front
            qint64 split = QRandomGenerator::global()->bounded(chunk_size);
            while (split == 0)
                split = QRandomGenerator::global()->bounded(chunk_size);

            QVERIFY(stream->seek(idx * tc.getStats().chunk_size + split));
            qint64 ret = stream->read(tmp.data() + split, chunk_size - split);
            QVERIFY(ret == (chunk_size - split));
            written += ret;

            QVERIFY(stream->seek(idx * tc.getStats().chunk_size));
            ret = stream->read(tmp.data(), split);
            QVERIFY(ret == split);
            written += ret;

            // Verify the hash
            bt::SHA1Hash hash = bt::SHA1Hash::generate((const bt::Uint8 *)tmp.data(), tmp.size());
            QVERIFY(hash == tc.getTorrent().getHash(idx));
            idx++;
        }

        stream->close();
        Out(SYS_GEN | LOG_DEBUG) << "End: testSeek() " << endl;
    }

    void testRandomAccess()
    {
        Out(SYS_GEN | LOG_DEBUG) << "Begin: testRandomAccess() " << endl;
        bt::TorrentFileStream::Ptr stream = tc.createTorrentFileStream(0, false, this);
        QVERIFY(stream);
        QVERIFY(!stream->open(QIODevice::ReadWrite));
        QVERIFY(stream->open(QIODevice::ReadOnly));

        // Read an area of around 5 chunks in at a random location
        // And verify the 3 middle chunks
        qint64 range_size = tc.getStats().chunk_size * 5;
        qint64 off = QRandomGenerator::global()->bounded(100) / 100.0 * (tc.getStats().total_bytes - range_size);
        QVERIFY(stream->seek(off));

        Out(SYS_GEN | LOG_DEBUG) << "Reading random range" << endl;
        Out(SYS_GEN | LOG_DEBUG) << "range_size = " << range_size << endl;
        Out(SYS_GEN | LOG_DEBUG) << "off = " << off << endl;

        QByteArray range(range_size, 0);
        qint64 bytes_read = 0;
        while (bytes_read < range_size) {
            qint64 ret = stream->read(range.data() + bytes_read, range_size - bytes_read);
            Out(SYS_GEN | LOG_DEBUG) << "ret = " << ret << endl;
            Out(SYS_GEN | LOG_DEBUG) << "read = " << bytes_read << endl;
            QVERIFY(ret > 0);
            bytes_read += ret;
        }

        {
            QFile fptr(stream->path());
            QVERIFY(fptr.open(QIODevice::ReadOnly));
            QByteArray tmp(range_size, 0);
            fptr.seek(off);
            QVERIFY(fptr.read(tmp.data(), range_size) == range_size);
            QVERIFY(tmp == range);
        }

        QVERIFY(bytes_read == range_size);

        // Calculate the offset of the first chunk in range
        qint64 chunk_idx = off / tc.getStats().chunk_size;
        if (off % tc.getStats().chunk_size != 0)
            chunk_idx++;
        qint64 chunk_off = chunk_idx * tc.getStats().chunk_size - off;

        Out(SYS_GEN | LOG_DEBUG) << "chunk_idx = " << chunk_idx << endl;
        Out(SYS_GEN | LOG_DEBUG) << "chunk_off = " << chunk_off << endl;

        // Verify the hashes
        for (int i = 0; i < 3; i++) {
            bt::SHA1Hash hash = bt::SHA1Hash::generate((const bt::Uint8 *)range.data() + chunk_off, tc.getStats().chunk_size);

            Out(SYS_GEN | LOG_DEBUG) << "chash = " << hash.toString() << endl;
            Out(SYS_GEN | LOG_DEBUG) << "whash = " << tc.getTorrent().getHash(chunk_idx).toString() << endl;
            QVERIFY(hash == tc.getTorrent().getHash(chunk_idx));
            chunk_idx++;
            chunk_off += tc.getStats().chunk_size;
        }

        stream->close();
        Out(SYS_GEN | LOG_DEBUG) << "End: testRandomAccess() " << endl;
    }

    void testMultiSeek()
    {
        Out(SYS_GEN | LOG_DEBUG) << "Begin: testMultiSeek() " << endl;
        bt::TorrentFileStream::Ptr stream = tc.createTorrentFileStream(0, false, this);
        QVERIFY(stream);
        QVERIFY(!stream->open(QIODevice::ReadWrite));
        QVERIFY(stream->open(QIODevice::ReadOnly));

        QFile fptr(stream->path());
        QVERIFY(fptr.open(QIODevice::ReadOnly));
        for (int i = 0; i < 20; i++) {
            qint64 off = QRandomGenerator::global()->bounded(TEST_FILE_SIZE - 100);
            // Seek to a random location
            QVERIFY(stream->seek(off));
            QByteArray tmp(100, 0);
            QVERIFY(stream->read(tmp.data(), 100) == 100);

            // Verify those
            QByteArray tmp2(100, 0);
            fptr.seek(off);
            QVERIFY(fptr.read(tmp2.data(), 100) == 100);
            QVERIFY(tmp == tmp2);
        }

        stream->close();
        Out(SYS_GEN | LOG_DEBUG) << "End: testMultiSeek() " << endl;
    }

    void testStreamingCreate()
    {
        bt::TorrentFileStream::Ptr a = tc.createTorrentFileStream(0, true, this);
        QVERIFY(a);
        bt::TorrentFileStream::Ptr b = tc.createTorrentFileStream(0, true, this);
        QVERIFY(!b);
        a.clear();
        b = tc.createTorrentFileStream(0, true, this);
        QVERIFY(b);
    }

    void testSeekToUndownloadedSection()
    {
        bt::TorrentFileStream::Ptr a = incomplete_tc.createTorrentFileStream(0, true, this);
        QVERIFY(incomplete_tc.getStats().completed == false);
        QVERIFY(a->seek(TEST_FILE_SIZE / 2));
        QVERIFY(a->bytesAvailable() == 0);
    }

private:
    DummyTorrentCreator creator;
    DummyTorrentCreator creator2;
    bt::TorrentControl tc;
    bt::TorrentControl incomplete_tc;
};

QTEST_MAIN(TorrentFileStreamTest)

#include "torrentfilestreamtest.moc"
