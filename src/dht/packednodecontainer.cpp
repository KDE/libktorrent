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

#include "packednodecontainer.h"

namespace dht
{
PackedNodeContainer::PackedNodeContainer()
{
}
PackedNodeContainer::~PackedNodeContainer()
{
}

void PackedNodeContainer::addNode(const QByteArray &a)
{
    if (a.size() == 26)
        nodes.append(a);
    else
        nodes6.append(a);
}

}
