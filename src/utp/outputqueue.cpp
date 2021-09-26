/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
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

#include "outputqueue.h"
#include <QSet>
#include <net/socket.h>
#include <util/log.h>

using namespace bt;

namespace utp
{
OutputQueue::OutputQueue()
    : mutex(QMutex::Recursive)
{
}

OutputQueue::~OutputQueue()
{
}

int OutputQueue::add(const PacketBuffer &packet, Connection::WPtr conn)
{
    QMutexLocker lock(&mutex);
    queue.push_back(Entry(packet, conn));
    return queue.size();
}

void OutputQueue::send(net::ServerSocket *sock)
{
    QList<Connection::WPtr> to_close;
    QMutexLocker lock(&mutex);
    try {
        // Keep sending until the output queue is empty or the socket
        // can't handle the data anymore
        while (!queue.empty()) {
            Entry &packet = queue.front();
            Connection::Ptr conn = packet.conn.toStrongRef();
            if (!conn) {
                queue.pop_front();
                continue;
            }

            int ret = sock->sendTo(packet.data.data(), packet.data.bufferSize(), conn->remoteAddress());
            if (ret == net::SEND_WOULD_BLOCK)
                break;
            else if (ret == net::SEND_FAILURE) {
                // Kill the connection of this packet
                to_close.append(packet.conn);
                queue.pop_front();
            } else
                queue.pop_front();
        }
    } catch (Connection::TransmissionError &err) {
        Out(SYS_UTP | LOG_NOTICE) << "UTP: " << err.location << endl;
    }
    sock->setWriteNotificationsEnabled(!queue.empty());
    lock.unlock(); // unlock, so we can't get deadlocked in any subsequent close calls

    for (const utp::Connection::WPtr &conn : qAsConst(to_close)) {
        Connection::Ptr c = conn.toStrongRef();
        if (c)
            c->close();
    }
}

}
