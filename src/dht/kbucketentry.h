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

#ifndef DHT_KBUCKETENTRY_H
#define DHT_KBUCKETENTRY_H

#include <dht/key.h>
#include <net/address.h>
#include <set>

namespace dht
{
/**
 * @author Joris Guisson
 *
 * Entry in a KBucket, it basically contains an ip_address of a node,
 * the udp port of the node and a node_id.
 */
class KBucketEntry
{
public:
    /**
     * Constructor, sets everything to 0.
     * @return
     */
    KBucketEntry();

    /**
     * Constructor, set the ip, port and key
     * @param addr socket address
     * @param id ID of node
     */
    KBucketEntry(const net::Address &addr, const Key &id);

    /**
     * Copy constructor.
     * @param other KBucketEntry to copy
     * @return
     */
    KBucketEntry(const KBucketEntry &other);

    /// Destructor
    virtual ~KBucketEntry();

    /**
     * Assignment operator.
     * @param other Node to copy
     * @return this KBucketEntry
     */
    KBucketEntry &operator=(const KBucketEntry &other);

    /// Equality operator
    bool operator==(const KBucketEntry &entry) const;

    /// Get the socket address of the node
    const net::Address &getAddress() const
    {
        return addr;
    }

    /// Get it's ID
    const Key &getID() const
    {
        return node_id;
    }

    /// Is this node a good node
    bool isGood() const;

    /// Is this node questionable (haven't heard from it in the last 15 minutes)
    bool isQuestionable() const;

    /// Is it a bad node. (Hasn't responded to a query
    bool isBad() const;

    /// Signal the entry that the peer has responded
    void hasResponded();

    /// A request timed out
    void requestTimeout()
    {
        failed_queries++;
    }

    /// The entry has been pinged because it is questionable
    void onPingQuestionable()
    {
        questionable_pings++;
    }

    /// The null entry
    static KBucketEntry null;

    /// < operator
    bool operator<(const KBucketEntry &entry) const;

private:
    net::Address addr;
    Key node_id;
    bt::TimeStamp last_responded;
    bt::Uint32 failed_queries;
    bt::Uint32 questionable_pings;
};

class KBucketEntrySet : public std::set<KBucketEntry>
{
public:
    KBucketEntrySet()
    {
    }
    virtual ~KBucketEntrySet()
    {
    }

    bool contains(const KBucketEntry &entry) const
    {
        return find(entry) != end();
    }
};

}

#endif // DHT_KBUCKETENTRY_H
