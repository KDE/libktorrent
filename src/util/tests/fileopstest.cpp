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
#include <QObject>
#include <QtTest>
#include <solid/device.h>
#include <solid/storageaccess.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

class FileOpsTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("fileopstest.log");
    }

    void cleanupTestCase()
    {
    }

    void testMountPointDetermination()
    {
        const QList<Solid::Device> devs = Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess);
        QString mountpoint;

        for (const Solid::Device &dev : devs) {
            const Solid::StorageAccess *sa = dev.as<Solid::StorageAccess>();
            if (sa->isAccessible()) {
                QVERIFY(bt::MountPoint(sa->filePath()) == sa->filePath());

                QString path = sa->filePath() + "/some/random/path/test.foobar";
                Out(SYS_GEN | LOG_DEBUG) << "Testing " << path << endl;
                QVERIFY(bt::MountPoint(path) == sa->filePath());
            }
        }
    }
};

QTEST_MAIN(FileOpsTest)

#include "fileopstest.moc"
