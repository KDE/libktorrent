/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QRandomGenerator>
#include <QTest>

#include <ctime>

#include <util/circularbuffer.h>
#include <util/log.h>
#include <util/pipe.h>

using namespace bt;

constexpr std::array<bt::Uint8, 13> data{0xFF};
constexpr std::array<bt::Uint8, 6> data2{0xEE};

class CircularBufferTest : public QEventLoop
{
    Q_OBJECT
public:
private Q_SLOTS:

    void testWrite()
    {
        bt::CircularBuffer wnd(20);
        QCOMPARE(wnd.capacity(), 20);
        QCOMPARE(wnd.size(), 0);

        QCOMPARE(wnd.write(data), 13);
        QCOMPARE(wnd.size(), 13);

        QCOMPARE(wnd.write(data2), 6);
        QCOMPARE(wnd.size(), 19);

        QCOMPARE(wnd.write(data2), 1);
        QCOMPARE(wnd.size(), 20);
    }

    void testRead()
    {
        bt::CircularBuffer wnd(20);
        QCOMPARE(wnd.capacity(), 20);
        QCOMPARE(wnd.write(data), 13);
        QCOMPARE(wnd.write(data2), 6);

        bt::Uint8 ret[19];
        QCOMPARE(wnd.read(ret, 19), 19);
        QCOMPARE(wnd.size(), 0);
        QCOMPARE(memcmp(ret, data.data(), 13), 0);
        QCOMPARE(memcmp(ret + 13, data2.data(), 6), 0);

        QCOMPARE(wnd.write(data), 13);
        QCOMPARE(wnd.size(), 13);

        QCOMPARE(wnd.write(data2), 6);
        QCOMPARE(wnd.size(), 19);

        QCOMPARE(wnd.read(ret, 19), 19);
        QCOMPARE(wnd.size(), 0);
        QCOMPARE(memcmp(ret, data.data(), 13), 0);
        QCOMPARE(memcmp(ret + 13, data2.data(), 6), 0);
    }

    void testIntensively()
    {
        CircularBuffer cbuf(20);

        for (int i = 0; i < 1000; i++) {
            Uint32 r = 1 + QRandomGenerator::global()->bounded(13);
            Uint32 expected = r;
            if (expected + cbuf.size() >= 20) {
                expected = 20 - cbuf.size();
            }

            QCOMPARE(cbuf.write(QByteArrayView{data}.first(r)), expected);

            bt::Uint8 ret[20];
            memset(ret, 0, 20);
            r = 1 + QRandomGenerator::global()->bounded(20);
            expected = qMin(r, cbuf.size());
            QCOMPARE(cbuf.read(ret, expected), expected);
        }
    }

    void testErrors()
    {
        CircularBuffer cbuf(20);
        constexpr std::array<bt::Uint8, 40> too_much{};
        QCOMPARE(cbuf.write(too_much), 20);
        QCOMPARE(cbuf.write(too_much), 0);
    }
};

QTEST_MAIN(CircularBufferTest)

#include "circularbuffertest.moc"
