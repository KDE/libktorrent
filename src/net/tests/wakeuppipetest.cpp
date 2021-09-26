/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QtTest>
#include <net/wakeuppipe.h>
#include <util/log.h>
#include <util/pipe.h>

using namespace net;
using namespace bt;

class WakeUpPipeTest : public QEventLoop
{
    Q_OBJECT
public:
public Q_SLOTS:

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("wakeuppipetest.log");
    }

    void cleanupTestCase()
    {
    }

    void testWakeUp()
    {
        Poll poll;
        WakeUpPipe::Ptr p(new WakeUpPipe);
        p->wakeUp();

        poll.add(p);
        QVERIFY(poll.poll() > 0);
    }

    void testEmptyWakeUp()
    {
        WakeUpPipe::Ptr p(new WakeUpPipe);
        Poll poll;
        poll.add(p);
        QVERIFY(poll.poll(100) == 0);
    }

private:
};

QTEST_MAIN(WakeUpPipeTest)

#include "wakeuppipetest.moc"
