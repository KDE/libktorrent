/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <util/bufferpool.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

class BufferPoolTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"bufferpooltest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void testPool()
    {
        bt::BufferPool::Ptr pool(new bt::BufferPool());
        pool->setWeakPointer(pool.toWeakRef());

        auto a = pool->get(1000);
        QVERIFY(a);
        QVERIFY(a->size() == 1000);
        QVERIFY(a->capacity() == 1000);
        a.reset();

        a = pool->get(500);
        QVERIFY(a);
        QVERIFY(a->size() == 500);
        QVERIFY(a->capacity() == 1000);

        auto b = pool->get(2000);
        QVERIFY(b);
        QVERIFY(b->size() == 2000);
        QVERIFY(b->capacity() == 2000);
    }
};

QTEST_MAIN(BufferPoolTest)

#include "bufferpooltest.moc"
