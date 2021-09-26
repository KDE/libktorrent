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
#include "rpcmsg.h"
#include <bcodec/bnode.h>
#include <util/error.h>

using namespace bt;

namespace dht
{
RPCMsg::RPCMsg()
    : mtid(nullptr)
    , method(NONE)
    , type(INVALID)
{
}

RPCMsg::RPCMsg(const QByteArray &mtid, Method m, Type type, const Key &id)
    : mtid(mtid)
    , method(m)
    , type(type)
    , id(id)
{
}

RPCMsg::~RPCMsg()
{
}

void RPCMsg::parse(bt::BDictNode *dict)
{
    mtid = dict->getByteArray(TID);
    if (mtid.isEmpty())
        throw bt::Error("Invalid DHT transaction ID");

    QString t = dict->getString(TYP, nullptr);
    if (t == REQ) {
        type = REQ_MSG;
        BDictNode *args = dict->getDict(ARG);
        if (!args)
            return;

        id = Key(args->getByteArray("id"));
    } else if (t == RSP) {
        type = RSP_MSG;
        BDictNode *args = dict->getDict(RSP);
        if (!args)
            return;

        id = Key(args->getByteArray("id"));
    } else if (t == ERR_DHT)
        type = ERR_MSG;
    else
        throw bt::Error(QString("Unknown message type %1").arg(t));
}
}
