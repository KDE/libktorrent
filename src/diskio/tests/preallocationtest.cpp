/***************************************************************************
 *   Copyright (C) 2010 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include <QtTest>
#include <QLocale>
#include <util/log.h>
#include <util/error.h>
#include <util/functions.h>
#include <util/fileops.h>
#include <testlib/utils.h>
#include <testlib/dummytorrentcreator.h>
#include <torrent/torrent.h>
#include <diskio/multifilecache.h>
#include <diskio/preallocationthread.h>
#include <diskio/singlefilecache.h>

const bt::Uint64 TEST_FILE_SIZE = 15 * 1024 * 1024;

using namespace bt;

class PreallocationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QLocale::setDefault(QLocale("main"));
        bt::InitLibKTorrent();
        bt::InitLog("preallocationtest.log", false, true);
        QMap<QString, bt::Uint64> files;

        files["aaa.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["bbb.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files["ccc.avi"] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);

        try {
            QVERIFY(multi_creator.createMultiFileTorrent(files, "movies"));
            Out(SYS_GEN | LOG_DEBUG) << "Created " << multi_creator.torrentPath() << endl;
            multi_tor.load(bt::LoadFile(multi_creator.torrentPath()), false);

            // Truncate the files so we can preallocate them again
            for (QMap<QString, bt::Uint64>::const_iterator i = files.cbegin(); i != files.cend(); i++) {
                bt::TruncateFile(multi_creator.dataPath() + i.key(), 0);
            }
        } catch (bt::Error& err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << multi_creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }

        try {
            QVERIFY(single_creator.createSingleFileTorrent(RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE), "bla.avi"));
            Out(SYS_GEN | LOG_DEBUG) << "Created " << single_creator.torrentPath() << endl;
            single_tor.load(bt::LoadFile(single_creator.torrentPath()), false);

            // Truncate the file so we can preallocate them again
            bt::TruncateFile(single_creator.dataPath(), 0);
        } catch (bt::Error& err) {
            Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << single_creator.torrentPath() << endl;
            QFAIL("Torrent load failure");
        }
    }

    void cleanupTestCase()
    {
    }

    void testPreallocationMultiFileCache()
    {
        bt::MultiFileCache cache(multi_tor, multi_creator.tempPath(), multi_creator.dataPath(), true);
        cache.loadFileMap();
        cache.setPreallocateFully(true);
        cache.open();

        PreallocationThread prealloc;
        cache.preparePreallocation(&prealloc);
        prealloc.run();

        if (!prealloc.errorMessage().isEmpty())
            Out(SYS_GEN | LOG_DEBUG) << "Preallocation failed: " << prealloc.errorMessage() << endl;

        Out(SYS_GEN | LOG_DEBUG) << "bw: " << prealloc.bytesWritten() << ", ts: " << multi_tor.getTotalSize() << endl;
        QVERIFY(prealloc.errorHappened() == false);
        QVERIFY(prealloc.bytesWritten() == multi_tor.getTotalSize());

        for (bt::Uint32 i = 0; i < multi_tor.getNumFiles(); i++) {
            QVERIFY(bt::FileSize(multi_tor.getFile(i).getPathOnDisk()) == multi_tor.getFile(i).getSize());
        }
    }

    void testPreallocationSingleFileCache()
    {
        QFileInfo info(single_creator.dataPath());
        bt::SingleFileCache cache(single_tor, single_creator.tempPath(), info.absoluteDir().absolutePath() + bt::DirSeparator());
        cache.loadFileMap();
        cache.setPreallocateFully(true);
        cache.open();

        PreallocationThread prealloc;
        cache.preparePreallocation(&prealloc);
        prealloc.run();

        if (!prealloc.errorMessage().isEmpty())
            Out(SYS_GEN | LOG_DEBUG) << "Preallocation failed: " << prealloc.errorMessage() << endl;

        Out(SYS_GEN | LOG_DEBUG) << "bw: " << prealloc.bytesWritten() << ", ts: " << single_tor.getTotalSize() << endl;
        QVERIFY(prealloc.errorHappened() == false);
        QVERIFY(prealloc.bytesWritten() == single_tor.getTotalSize());
        QVERIFY(bt::FileSize(single_creator.dataPath()) == single_tor.getTotalSize());
    }

private:
    DummyTorrentCreator multi_creator;
    bt::Torrent multi_tor;
    DummyTorrentCreator single_creator;
    bt::Torrent single_tor;
};


QTEST_MAIN(PreallocationTest)

#include "preallocationtest.moc"
