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
#ifndef BTBADPEERSLIST_H
#define BTBADPEERSLIST_H

#include <QStringList>
#include <interfaces/blocklistinterface.h>

namespace bt
{
/**
    Blocklist to keep track of bad peers.
*/
class BadPeersList : public BlockListInterface
{
public:
    BadPeersList();
    ~BadPeersList() override;

    bool blocked(const net::Address &addr) const override;

    /// Add a bad peer to the list
    void addBadPeer(const QString &ip);

private:
    QStringList bad_peers;
};

}

#endif
