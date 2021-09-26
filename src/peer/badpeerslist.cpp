/*
    SPDX-FileCopyrightText: 2008 Joris Guisson and Ivan Vasic
    joris.guisson@gmail.com
    ivasic@gmail.com

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
#include "badpeerslist.h"
#include <net/address.h>

namespace bt
{
BadPeersList::BadPeersList()
{
}

BadPeersList::~BadPeersList()
{
}

bool BadPeersList::blocked(const net::Address &addr) const
{
    return bad_peers.contains(addr.toString());
}

void BadPeersList::addBadPeer(const QString &ip)
{
    bad_peers << ip;
}

}
