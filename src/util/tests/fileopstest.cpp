/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
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
