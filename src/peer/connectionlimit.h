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

#ifndef BT_CONNECTIONLIMIT_H
#define BT_CONNECTIONLIMIT_H

#include <QMap>
#include <QSharedPointer>
#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/sha1hash.h>

namespace bt
{
/**
 * Maintains the connection limit. It uses a Token for that.
 */
class KTORRENT_EXPORT ConnectionLimit
{
public:
    ConnectionLimit();
    virtual ~ConnectionLimit();

    /// Get the total number of connections currently in use
    bt::Uint32 totalConnections() const
    {
        return global_total;
    }

    /**
     * Set the connection limits
     * @param global_limit Global limit
     * @param torrent_limit Per torrent limit
     **/
    void setLimits(bt::Uint32 global_limit, bt::Uint32 torrent_limit);

    /**
     * Token representing the allowance to open a connection.
     * When the token is destroyed, it will be automatically released.
     */
    class Token
    {
    public:
        Token(ConnectionLimit &limit, const bt::SHA1Hash &hash);
        ~Token();

        /// Get the info hash
        const bt::SHA1Hash &infoHash() const
        {
            return hash;
        }

        typedef QSharedPointer<Token> Ptr;

    private:
        ConnectionLimit &limit;
        bt::SHA1Hash hash;
    };

    /**
     * Request a token for a given torrent
     * @param hash Info hash of the torrent
     * @return ConnectionLimit::Token::Ptr a valid token if a connection can be opened, a 0 pointer if not
     **/
    Token::Ptr acquire(const SHA1Hash &hash);

protected:
    /**
     * Release one Token. Will be done by destructor of Token.
     * @param token The Token
     **/
    void release(const Token &token);

private:
    bt::Uint32 global_limit;
    bt::Uint32 global_total;
    bt::Uint32 torrent_limit;
    QMap<SHA1Hash, bt::Uint32> torrent_totals;
};

}

#endif // BT_CONNECTIONLIMIT_H
