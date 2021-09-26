/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <QLocale>
#include <QtTest>
#include <diskio/chunkmanager.h>
#include <diskio/piecedata.h>
#include <testlib/dummytorrentcreator.h>
#include <testlib/utils.h>
#include <torrent/torrentcontrol.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/sha1hashgen.h>
#include <util/signalcatcher.h>

using namespace bt;

const bt::Uint64 TEST_FILE_SIZE = 15 * 1024 * 1024;

class ChunkManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QLocale::setDefault(QLocale("main"));
        bt::InitLibKTorrent();
        bt::InitLog("chunkmanagertest.log", false, true);
        QMap<QString, bt::Uint64> files;

        files["aaa.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["bbb.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["ccc.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);

        QVERIFY(creator.createMultiFileTorrent(files, "movies"));

        Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << endl;
        try {
            tor.load(bt::LoadFile(creator.torrentPath()), false);
        } catch (bt::Error &err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }
    }

    void cleanupTestCase()
    {
    }

    void testPieceDataLoading()
    {
        ChunkManager cman(tor, creator.tempPath(), creator.dataPath(), true, nullptr);
        Chunk *c = cman.getChunk(0);
        QVERIFY(c);

        try {
            PieceData::Ptr ptr = c->getPiece(0, MAX_PIECE_LEN, true);
            QVERIFY(ptr && ptr->data() && ptr->inUse());

            PieceData::Ptr f = c->getPiece(0, c->getSize(), true);
            QVERIFY(f);

            f = PieceData::Ptr();
            QVERIFY(!f);

            cman.checkMemoryUsage();
            Uint8 tmp[20];
            QVERIFY(memcpy(tmp, ptr->data(), 20));

            memset(tmp, 0xFF, 20);
            QVERIFY(!ptr->writeable());
            ptr->write(tmp, 20);
            QFAIL("No exception thrown");
        } catch (bt::Error &err) {
        }
    }

#ifndef Q_CC_MSVC
    void testBusErrorHandling()
    {
        ChunkManager cman(tor, creator.tempPath(), creator.dataPath(), true, nullptr);
        Chunk *c = cman.getChunk(0);
        QVERIFY(c);

        PieceData::Ptr f = c->getPiece(0, c->getSize(), false);
        QVERIFY(f);
        QVERIFY(f->writeable());

        try {
            QString path = creator.dataPath() + tor.getFile(0).getPath();
            bt::TruncateFile(path, 0);

            Uint8 tmp[20];
            memset(tmp, 0xFF, 20);

            f->write(tmp, 20);
            QFAIL("No BusError thrown\n");
        } catch (bt::BusError &err) {
        } catch (bt::Error &err) {
            QFAIL("Truncate failed\n");
        }
    }

#endif

private:
    DummyTorrentCreator creator;
    bt::Torrent tor;
};

QTEST_MAIN(ChunkManagerTest)

#include "chunkmanagertest.moc"
