/*
    SPDX-FileCopyrightText: 2011 Joris Guisson
    joris.guisson@gmail.com

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

#include <QtTest>
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
