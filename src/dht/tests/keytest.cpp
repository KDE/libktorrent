/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QString>
#include <QTest>

#include <algorithm>
#include <dht/key.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

static bt::Uint8 HexCharToUint8(char c)
{
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    else if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    else if (c >= '0' && c <= '9')
        return c - '0';
    else
        return 0;
}

static dht::Key KeyFromHexString(const QString &str)
{
    bt::Uint8 result[20];
    std::fill(result, result + 20, 0);

    QString s = str.toLower();
    if (s.size() % 2 != 0)
        s.prepend('0'_L1);

    int j = 19;

    for (int i = s.size() - 1; i >= 0; i -= 2) {
        char left = s[i - 1].toLatin1();
        char right = s[i].toLatin1();
        result[j--] = (HexCharToUint8(left) << 4) | HexCharToUint8(right);
    }

    return dht::Key(result);
}

class KeyTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"keytest.log"_s, false, true);
    }

    void cleanupTestCase()
    {
    }

    void testAddition()
    {
        dht::Key a = KeyFromHexString(u"14455"_s);
        dht::Key b = KeyFromHexString(u"FFEEDD"_s);
        dht::Key c = a + b;
        QVERIFY(c == KeyFromHexString(u"1013332"_s));

        dht::Key d = a + 1;
        QVERIFY(d == KeyFromHexString(u"14456"_s));

        dht::Key e = KeyFromHexString(u"FFFF"_s) + 1;
        QVERIFY(e == KeyFromHexString(u"10000"_s));

        dht::Key f = KeyFromHexString(u"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"_s) + 1;
        QVERIFY(f == KeyFromHexString(u"0000000000000000000000000000000000000000"_s));

        dht::Key h = KeyFromHexString(u"FFFFFFFF00000000FFFFFFFF00000000FFFFFFFF"_s);
        dht::Key j = KeyFromHexString(u"0000000100000000000000010000000000000001"_s);
        dht::Key g = j + h;
        QVERIFY(g == KeyFromHexString(u"0000000000000001000000000000000100000000"_s));
    }

    void testSubtraction()
    {
        dht::Key a = KeyFromHexString(u"556677"_s);
        dht::Key b = KeyFromHexString(u"3384E6"_s);
        dht::Key c = a - b;
        QVERIFY(c == KeyFromHexString(u"21E191"_s));

        dht::Key d = KeyFromHexString(u"550077"_s);
        dht::Key e = KeyFromHexString(u"3384E6"_s);
        dht::Key f = d - e;
        QVERIFY(f == KeyFromHexString(u"217B91"_s));

        f = e - d;
        QVERIFY(f == KeyFromHexString(u"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFDE846F"_s));

        dht::Key g = KeyFromHexString(u"0000000000000001000000000000000100000000"_s);
        dht::Key j = KeyFromHexString(u"0000000100000000000000010000000000000001"_s);
        dht::Key h = g - j;
        QVERIFY(h == KeyFromHexString(u"FFFFFFFF00000000FFFFFFFF00000000FFFFFFFF"_s));
    }

    void testDivision()
    {
        dht::Key a = KeyFromHexString(u"550078"_s);
        dht::Key b = a / 2;
        QVERIFY(b == KeyFromHexString(u"2A803C"_s));

        dht::Key c = KeyFromHexString(u"0000000100000000000000010000000000000001"_s);
        dht::Key d = c / 32;
        QVERIFY(d == KeyFromHexString(u"0000000008000000000000000800000000000000"_s));
    }

    void testConversion()
    {
        dht::Key d = KeyFromHexString(u"0102030405060708090AFFFEFDFCFBFAF9F8F7F6"_s);
        QVERIFY(d.toString() == QLatin1String("0102030405060708090afffefdfcfbfaf9f8f7f6"));
    }
};

QTEST_MAIN(KeyTest)

#include "keytest.moc"
