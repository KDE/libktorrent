/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QRandomGenerator>
#include <QtTest>

#include <ctime>

#include <util/circularbuffer.h>
#include <util/log.h>
#include <util/pipe.h>

using namespace bt;

class CircularBufferTest : public QEventLoop
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        memset(data, 0xFF, 13);
        memset(data2, 0xEE, 6);
    }

    void cleanupTestCase()
    {
    }

    void testWrite()
    {
        bt::CircularBuffer wnd(20);
        QVERIFY(wnd.capacity() == 20);
        QVERIFY(wnd.size() == 0);

        QVERIFY(wnd.write(data, 13) == 13);
        QVERIFY(wnd.size() == 13);

        QVERIFY(wnd.write(data2, 6) == 6);
        QVERIFY(wnd.size() == 19);

        QVERIFY(wnd.write(data2, 6) == 1);
        QVERIFY(wnd.size() == 20);
    }

    void testRead()
    {
        bt::CircularBuffer wnd(20);
        QVERIFY(wnd.capacity() == 20);
        QVERIFY(wnd.write(data, 13) == 13);
        QVERIFY(wnd.write(data2, 6) == 6);

        bt::Uint8 ret[19];
        QVERIFY(wnd.read(ret, 19) == 19);
        QVERIFY(wnd.size() == 0);
        QVERIFY(memcmp(ret, data, 13) == 0);
        QVERIFY(memcmp(ret + 13, data2, 6) == 0);

        QVERIFY(wnd.write(data, 13) == 13);
        QVERIFY(wnd.size() == 13);

        QVERIFY(wnd.write(data2, 6) == 6);
        QVERIFY(wnd.size() == 19);

        QVERIFY(wnd.read(ret, 19) == 19);
        QVERIFY(wnd.size() == 0);
        QVERIFY(memcmp(ret, data, 13) == 0);
        QVERIFY(memcmp(ret + 13, data2, 6) == 0);
    }

    void testIntensively()
    {
        CircularBuffer cbuf(20);

        for (int i = 0; i < 1000; i++) {
            Uint32 r = 1 + QRandomGenerator::global()->bounded(20);
            Uint32 expected = r;
            if (expected + cbuf.size() >= 20)
                expected = 20 - cbuf.size();

            QVERIFY(cbuf.write(data, r) == expected);

            bt::Uint8 ret[20];
            memset(ret, 0, 20);
            r = 1 + QRandomGenerator::global()->bounded(20);
            expected = qMin(r, cbuf.size());
            QVERIFY(cbuf.read(ret, expected) == expected);
        }
    }

    void testErrors()
    {
        CircularBuffer cbuf(20);
        bt::Uint8 too_much[40];
        QVERIFY(cbuf.write(too_much, 40) == 20);
        QVERIFY(cbuf.write(too_much, 40) == 0);
    }

private:
    bt::Uint8 data[13];
    bt::Uint8 data2[6];
};

QTEST_MAIN(CircularBufferTest)

#include "circularbuffertest.moc"
