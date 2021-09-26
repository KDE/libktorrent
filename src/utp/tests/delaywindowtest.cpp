/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QRandomGenerator>
#include <QtTest>

#include <ctime>

#include <boost/scoped_array.hpp>

#include <util/functions.h>
#include <util/log.h>
#include <utp/delaywindow.h>

using namespace utp;
using namespace bt;

class DelayWindowTest : public QObject
{
    Q_OBJECT
public:
    DelayWindowTest(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("delaywindowtest.log", false, true);
    }

    void cleanupTestCase()
    {
    }

    void testWindow()
    {
        bt::Uint32 base_delay = MAX_DELAY;
        DelayWindow wnd;

        for (int i = 0; i < 100; i++) {
            bt::Uint32 val = QRandomGenerator::global()->generate();
            Header hdr;
            hdr.timestamp_difference_microseconds = val;
            if (val < base_delay)
                base_delay = val;

            bt::Uint32 ret = wnd.update(&hdr, bt::Now());
            QVERIFY(ret == base_delay);
        }
    }
#if 1
    void testPerformance()
    {
        const int SAMPLE_COUNT = 2400000;

        boost::scoped_array<bt::Uint32> delay_samples(new bt::Uint32[SAMPLE_COUNT]);
        for (int i = 0; i < SAMPLE_COUNT; i++)
            delay_samples[i] = QRandomGenerator::global()->bounded(1000000);

        boost::scoped_array<bt::Uint32> returned_delay_new(new bt::Uint32[SAMPLE_COUNT]);
        boost::scoped_array<bt::Uint32> returned_delay_old(new bt::Uint32[SAMPLE_COUNT]);
        boost::scoped_array<bt::Uint32> returned_delay_circular(new bt::Uint32[SAMPLE_COUNT]);

        {
            DelayWindow wnd;
            bt::TimeStamp start = bt::Now();
            for (int i = 0; i < SAMPLE_COUNT; i++) {
                Header hdr;
                hdr.timestamp_difference_microseconds = delay_samples[i];
                returned_delay_new[i] = wnd.update(&hdr, i);
            }
            bt::TimeStamp duration = bt::Now() - start;
            Out(SYS_GEN | LOG_DEBUG) << "New algorithm took: " << duration << endl;
        }
    }
#endif

    void testTimeout()
    {
        DelayWindow wnd;

        Header hdr;
        hdr.timestamp_difference_microseconds = 1000;
        bt::TimeStamp ts = 1000;
        QVERIFY(wnd.update(&hdr, ts) == 1000);

        hdr.timestamp_difference_microseconds = 2000;
        QVERIFY(wnd.update(&hdr, ts + 1000) == 1000);

        // Now simulate timeout, oldest must get removed
        hdr.timestamp_difference_microseconds = 3000;
        QVERIFY(wnd.update(&hdr, ts + utp::DELAY_WINDOW_SIZE + 1) == 2000);
    }

private:
};

QTEST_MAIN(DelayWindowTest)

#include "delaywindowtest.moc"
