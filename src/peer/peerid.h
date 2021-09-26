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
#ifndef BTPEERID_H
#define BTPEERID_H

#include <ktorrent_export.h>
#include <qstring.h>

namespace bt
{
/**
@author Joris Guisson
*/
class KTORRENT_EXPORT PeerID
{
    char id[20];
    QString client_name;

public:
    PeerID();
    PeerID(const char *pid);
    PeerID(const PeerID &pid);
    virtual ~PeerID();

    PeerID &operator=(const PeerID &pid);

    const char *data() const
    {
        return id;
    }

    QString toString() const;

    /**
     * Interprets the PeerID to figure out which client it is.
     * @author Ivan + Joris
     * @return The name of the client
     */
    QString identifyClient() const;

    friend bool operator==(const PeerID &a, const PeerID &b);
    friend bool operator!=(const PeerID &a, const PeerID &b);
    friend bool operator<(const PeerID &a, const PeerID &b);
};

}

#endif
