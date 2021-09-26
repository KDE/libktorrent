/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
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
#ifndef DHTNODELOOKUP_H
#define DHTNODELOOKUP_H

#include "key.h"
#include "task.h"

namespace dht
{
class Node;
class RPCServer;

/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Task to do a node lookup.
 */
class NodeLookup : public Task
{
public:
    NodeLookup(const dht::Key &node_id, RPCServer *rpc, Node *node, QObject *parent);
    ~NodeLookup() override;

    void update() override;
    void callFinished(RPCCall *c, RPCMsg::Ptr rsp) override;
    void callTimeout(RPCCall *c) override;

private:
    void handleNodes(const QByteArray &nodes, int ip_version);

private:
    dht::Key node_id;
    bt::Uint32 num_nodes_rsp;
};

}

#endif
