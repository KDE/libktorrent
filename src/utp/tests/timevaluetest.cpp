/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>
#include <util/log.h>
#include <utp/timevalue.h>

using namespace utp;
using namespace bt;
using namespace Qt::Literals::StringLiterals;

class TimeValueTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"timevaluetest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void testTimeValue()
    {
        TimeValue a(1, 500000);
        TimeValue b(3, 200000);

        QCOMPARE_GT(b, a);
        bt::Int64 diff = b - a;
        Out(SYS_GEN | LOG_DEBUG) << "diff = " << diff << endl;
        QCOMPARE(diff, 1700);

        QCOMPARE_GT(b, TimeValue(3, 100000));
        QCOMPARE_GE(b, TimeValue(3, 200000));
        QCOMPARE_LE(b, TimeValue(3, 200000));
        QCOMPARE_LT(b, TimeValue(3, 500000));
    }
};

QTEST_MAIN(TimeValueTest)

#include "timevaluetest.moc"
