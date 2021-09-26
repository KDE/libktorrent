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
#ifndef BTSERVER_H
#define BTSERVER_H

#include "globals.h"
#include <interfaces/serverinterface.h>
#include <ktorrent_export.h>
#include <qlist.h>
#include <qobject.h>

namespace bt
{
class PeerManager;

/**
 * @author Joris Guisson
 *
 * Class which listens for incoming connections.
 * Handles authentication and then hands of the new
 * connections to a PeerManager.
 *
 * All PeerManager's should register with this class when they
 * are created and should unregister when they are destroyed.
 */
class KTORRENT_EXPORT Server : public ServerInterface
{
    Q_OBJECT
public:
    Server();
    ~Server() override;

    bool changePort(Uint16 port) override;

private:
    class Private;
    Private *d;
};

}

#endif
