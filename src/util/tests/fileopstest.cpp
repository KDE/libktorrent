/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <QObject>
#include <QTest>

#include <Solid/Device>
#include <Solid/StorageAccess>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

class FileOpsTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"fileopstest.log"_s);
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

                QString path = sa->filePath() + "/some/random/path/test.foobar"_L1;
                Out(SYS_GEN | LOG_DEBUG) << "Testing " << path << endl;
                QVERIFY(bt::MountPoint(path) == sa->filePath());
            }
        }
    }
};

QTEST_MAIN(FileOpsTest)

#include "fileopstest.moc"
