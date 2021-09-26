/*
    SPDX-FileCopyrightText: 2012 by
    Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <QString>
#include <QtTest>
#include <algorithm>
#include <dht/key.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;

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
        s.prepend('0');

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
        bt::InitLog("keytest.log", false, true);
    }

    void cleanupTestCase()
    {
    }

    void testAddition()
    {
        dht::Key a = KeyFromHexString("14455");
        dht::Key b = KeyFromHexString("FFEEDD");
        dht::Key c = a + b;
        QVERIFY(c == KeyFromHexString("1013332"));

        dht::Key d = a + 1;
        QVERIFY(d == KeyFromHexString("14456"));

        dht::Key e = KeyFromHexString("FFFF") + 1;
        QVERIFY(e == KeyFromHexString("10000"));

        dht::Key f = KeyFromHexString("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF") + 1;
        QVERIFY(f == KeyFromHexString("0000000000000000000000000000000000000000"));

        dht::Key h = KeyFromHexString("FFFFFFFF00000000FFFFFFFF00000000FFFFFFFF");
        dht::Key j = KeyFromHexString("0000000100000000000000010000000000000001");
        dht::Key g = j + h;
        QVERIFY(g == KeyFromHexString("0000000000000001000000000000000100000000"));
    }

    void testSubtraction()
    {
        dht::Key a = KeyFromHexString("556677");
        dht::Key b = KeyFromHexString("3384E6");
        dht::Key c = a - b;
        QVERIFY(c == KeyFromHexString("21E191"));

        dht::Key d = KeyFromHexString("550077");
        dht::Key e = KeyFromHexString("3384E6");
        dht::Key f = d - e;
        QVERIFY(f == KeyFromHexString("217B91"));

        f = e - d;
        QVERIFY(f == KeyFromHexString("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFDE846F"));

        dht::Key g = KeyFromHexString("0000000000000001000000000000000100000000");
        dht::Key j = KeyFromHexString("0000000100000000000000010000000000000001");
        dht::Key h = g - j;
        QVERIFY(h == KeyFromHexString("FFFFFFFF00000000FFFFFFFF00000000FFFFFFFF"));
    }

    void testDivision()
    {
        dht::Key a = KeyFromHexString("550078");
        dht::Key b = a / 2;
        QVERIFY(b == KeyFromHexString("2A803C"));

        dht::Key c = KeyFromHexString("0000000100000000000000010000000000000001");
        dht::Key d = c / 32;
        QVERIFY(d == KeyFromHexString("0000000008000000000000000800000000000000"));
    }

    void testConversion()
    {
        dht::Key d = KeyFromHexString("0102030405060708090AFFFEFDFCFBFAF9F8F7F6");
        QVERIFY(d.toString() == QLatin1String("0102030405060708090afffefdfcfbfaf9f8f7f6"));
    }
};

QTEST_MAIN(KeyTest)

#include "keytest.moc"
