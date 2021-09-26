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
#include <util/circularbuffer.h>
#include <util/log.h>
#include <utp/packetbuffer.h>
#include <utp/utpprotocol.h>

using namespace bt;

class PacketBufferTest : public QObject
{
    Q_OBJECT
public:
    void initTestCase()
    {
        bt::InitLog("packetbuffertest.log");
    }

    void cleanupTestCase()
    {
    }

private Q_SLOTS:
    void testPacketBuffer()
    {
        bt::Uint8 tmp[200];
        memset(tmp, 0xFF, 200);

        bt::CircularBuffer cbuf;
        cbuf.write(tmp, 200);

        utp::PacketBuffer pbuf;
        QVERIFY(pbuf.fillData(cbuf, 200) == 200);
        QVERIFY(pbuf.payloadSize() == 200);
        QVERIFY(pbuf.bufferSize() == 200);
        QVERIFY(memcmp(pbuf.data(), tmp, 200) == 0);

        utp::Header hdr;
        hdr.ack_nr = 600;
        hdr.connection_id = 1777;
        hdr.extension = 0;
        hdr.seq_nr = 1000;
        hdr.timestamp_difference_microseconds = 0;
        hdr.timestamp_microseconds = 0;
        hdr.version = 0;
        hdr.wnd_size = 6000;

        pbuf.setHeader(hdr, 0);
        QVERIFY(pbuf.bufferSize() == 200 + utp::Header::size());
        QVERIFY(memcmp(pbuf.data() + utp::Header::size(), tmp, 200) == 0);
        QVERIFY(pbuf.payloadSize() == 200);

        utp::Header hdr2;
        hdr2.read(pbuf.data());
        QVERIFY(memcmp(&hdr, &hdr2, sizeof(utp::Header)) == 0);

        pbuf.setHeader(hdr, 4);
        QVERIFY(pbuf.bufferSize() == 200 + utp::Header::size() + 4);
        QVERIFY(memcmp(pbuf.data() + utp::Header::size() + 4, tmp, 200) == 0);
        QVERIFY(pbuf.payloadSize() == 200);

        utp::Header hdr3;
        hdr3.read(pbuf.data());
        QVERIFY(memcmp(&hdr, &hdr3, sizeof(utp::Header)) == 0);
    }

    void testData()
    {
        bt::Uint8 tmp[200];
        memset(tmp, 0xFF, 200);

        utp::PacketBuffer pbuf;
        pbuf.fillData(tmp, 200);
        QVERIFY(pbuf.bufferSize() == 200);
        QVERIFY(pbuf.payloadSize() == 200);
        QVERIFY(memcmp(pbuf.data(), tmp, 200) == 0);
    }
};

QTEST_MAIN(PacketBufferTest)

#include "packetbuffertest.moc"
