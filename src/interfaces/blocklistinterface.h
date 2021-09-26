/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
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

#ifndef IPBLOCKINGINTERFACE_H
#define IPBLOCKINGINTERFACE_H

#include <ktorrent_export.h>

namespace net
{
class Address;
}

namespace bt
{
/**
 * @author Ivan Vasic
 * @brief Base class for BlockLists
 */
class KTORRENT_EXPORT BlockListInterface
{
public:
    BlockListInterface();
    virtual ~BlockListInterface();

    /**
     * This function checks if IP is blocked
     * @return TRUE if IP should be blocked. FALSE otherwise
     * @arg addr Address of the peer
     */
    virtual bool blocked(const net::Address &addr) const = 0;
};
}
#endif
