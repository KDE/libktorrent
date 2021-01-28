/***************************************************************************
 *   Copyright (C) 2011 by Joris Guisson                                   *
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
#include <util/bufferpool.h>
#include <util/log.h>

class BufferPoolTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("bufferpooltest.log");
    }

    void cleanupTestCase()
    {
    }

    void testPool()
    {
        bt::BufferPool::Ptr pool(new bt::BufferPool());
        pool->setWeakPointer(pool.toWeakRef());

        bt::Buffer::Ptr a = pool->get(1000);
        QVERIFY(a);
        QVERIFY(a->size() == 1000);
        QVERIFY(a->capacity() == 1000);
        a.clear();

        a = pool->get(500);
        QVERIFY(a);
        QVERIFY(a->size() == 500);
        QVERIFY(a->capacity() == 1000);

        bt::Buffer::Ptr b = pool->get(2000);
        QVERIFY(b);
        QVERIFY(b->size() == 2000);
        QVERIFY(b->capacity() == 2000);
    }
};

QTEST_MAIN(BufferPoolTest)

#include "bufferpooltest.moc"
