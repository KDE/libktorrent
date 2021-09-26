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
#include "rpccall.h"
#include "dht.h"
#include "rpcmsg.h"

namespace dht
{
RPCCallListener::RPCCallListener(QObject *parent)
    : QObject(parent)
{
}

RPCCallListener::~RPCCallListener()
{
}

RPCCall::RPCCall(dht::RPCMsg::Ptr msg, bool queued)
    : msg(msg)
    , queued(queued)
{
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &RPCCall::onTimeout);
    if (!queued)
        timer.start(30 * 1000);
}

RPCCall::~RPCCall()
{
}

void RPCCall::start()
{
    queued = false;
    timer.start(30 * 1000);
}

void RPCCall::onTimeout()
{
    timeout(this);
}

void RPCCall::response(RPCMsg::Ptr rsp)
{
    response(this, rsp);
}

Method RPCCall::getMsgMethod() const
{
    if (msg)
        return msg->getMethod();
    else
        return dht::NONE;
}

void RPCCall::addListener(RPCCallListener *cl)
{
    connect(this, qOverload<RPCCall *, RPCMsg::Ptr>(&RPCCall::response), cl, &RPCCallListener::onResponse);
    connect(this, &RPCCall::timeout, cl, &RPCCallListener::onTimeout);
}

}
