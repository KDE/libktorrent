/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>
#include <util/log.h>
#include <utp/timevalue.h>

using namespace utp;
using namespace bt;

class TimeValueTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("timevaluetest.log");
    }

    void cleanupTestCase()
    {
    }

    void testTimeValue()
    {
        TimeValue a(1, 500000);
        TimeValue b(3, 200000);

        QVERIFY(b >= a);
        bt::Int64 diff = b - a;
        Out(SYS_GEN | LOG_DEBUG) << "diff = " << diff << endl;
        QVERIFY(b - a == 1700);

        QVERIFY(b >= TimeValue(3, 200000));
        QVERIFY(b >= TimeValue(3, 100000));
        QVERIFY(b <= TimeValue(3, 200000));
        QVERIFY(b <= TimeValue(3, 500000));

        QVERIFY(b < TimeValue(3, 500000));
        QVERIFY(b > TimeValue(3, 100000));
    }
};

QTEST_MAIN(TimeValueTest)

#include "timevaluetest.moc"
