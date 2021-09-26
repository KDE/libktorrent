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
#ifndef BTCACHEFACTORY_H
#define BTCACHEFACTORY_H

#include <QString>
#include <ktorrent_export.h>

namespace bt
{
class Cache;
class Torrent;

/**
 * Factory to create Cache objects. If you want a custom Cache you need to derive from this class
 * and implement the create method to create your own custom Caches.
 * @author Joris Guisson
 */
class KTORRENT_EXPORT CacheFactory
{
public:
    CacheFactory();
    virtual ~CacheFactory();

    /**
     * Create a custom Cache
     * @param tor The Torrent
     * @param tmpdir The temporary directory (should be used to store information about the torrent)
     * @param datadir The data directory, where to store the data
     * @return
     */
    virtual Cache *create(Torrent &tor, const QString &tmpdir, const QString &datadir) = 0;
};

}

#endif
