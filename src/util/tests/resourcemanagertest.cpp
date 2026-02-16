/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QRandomGenerator>
#include <QTest>

#include <util/functions.h>
#include <util/log.h>
#include <util/resourcemanager.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

class TestResource;

static TestResource *last_acquired = nullptr;

class TestResource : public bt::Resource
{
public:
    TestResource(ResourceManager *rman, const QString &group)
        : Resource(rman, group)
        , acq(false)
    {
    }

    void acquired() override
    {
        Out(SYS_GEN | LOG_DEBUG) << "Resource of " << groupName() << " acquired" << endl;
        acq = true;
        last_acquired = this;
    }

    bool acq;
};

class ResourceManagerTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"resourcemanagertest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void testSingleClass()
    {
        Out(SYS_GEN | LOG_DEBUG) << "testSingleClass" << endl;
        ResourceManager rm(4);

        QList<TestResource *> tr;
        for (int i = 0; i < 8; i++) {
            TestResource *r = new TestResource(&rm, u"test"_s);
            tr.append(r);
            rm.add(r);
            // The first 4 should get acquired
            QVERIFY(r->acq == (i < 4));
        }

        for (int i = 0; i < 4; i++) {
            delete tr.takeFirst();
            QVERIFY(tr.at(3)->acq); // The next availabe one should now be acquired
        }
        qDeleteAll(tr);
    }

    void testMultiClass()
    {
        Out(SYS_GEN | LOG_DEBUG) << "testMultiClass" << endl;
        ResourceManager rm(4);
        const QStringList classes = {u"aaa"_s, u"bbb"_s, u"ccc"_s, u"ddd"_s};

        // 4 resources for each class
        QList<TestResource *> tr;
        for (int i = 0; i < 16; i++) {
            TestResource *r = new TestResource(&rm, classes[i % 4]);
            tr.append(r);
            rm.add(r);
            // The first 4 should get acquired
            QVERIFY(r->acq == (i < 4));
        }

        QString last_group;
        for (int i = 0; i < 12; i++) {
            Resource *r = tr.takeFirst();
            QString g = r->groupName();
            delete r;
            QVERIFY(tr.at(3)->acq); // The next availabe one should now be acquired
            QVERIFY(g != last_group);
            last_group = g;
        }
        qDeleteAll(tr);
    }

    void testFullyRandom()
    {
        Out(SYS_GEN | LOG_DEBUG) << "testFullyRandom" << endl;
        ResourceManager rm(4);
        const QStringList classes = {u"aaa"_s, u"bbb"_s, u"ccc"_s, u"ddd"_s};

        // A random amount of resources for each class
        QList<TestResource *> tr;

        Uint32 num_acquired = 0;
        for (int i = 0; i < 500; i++) {
            TestResource *r = new TestResource(&rm, classes[QRandomGenerator::global()->bounded(4)]);
            tr.append(r);
            rm.add(r);
            if (r->acq) {
                num_acquired++;
            }
        }

        QVERIFY(num_acquired == 4);

        QString last_acquired_group;
        for (int i = 0; i < 496; i++) {
            tr.removeAll(last_acquired);
            delete last_acquired;
            QVERIFY(last_acquired);
            if (last_acquired->groupName() == last_acquired_group) {
                // This is only possible if there are no other groups which are still pending
                for (TestResource *r : std::as_const(tr)) {
                    if (!r->acq) {
                        QVERIFY(r->groupName() == last_acquired_group);
                    }
                }
            }
            last_acquired_group = last_acquired->groupName();
        }
        qDeleteAll(tr);
    }
};

QTEST_MAIN(ResourceManagerTest)

#include "resourcemanagertest.moc"
