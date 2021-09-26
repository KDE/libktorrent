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
#ifndef BTSINGLEDATACHECKER_H
#define BTSINGLEDATACHECKER_H

#include "datachecker.h"

namespace bt
{
/**
 * @author Joris Guisson
 *
 * Data checker for single file torrents.
 */
class KTORRENT_EXPORT SingleDataChecker : public DataChecker
{
public:
    SingleDataChecker(bt::Uint32 from, bt::Uint32 to);
    ~SingleDataChecker() override;

    void check(const QString &path, const Torrent &tor, const QString &dnddir, const BitSet &current_status) override;
};

}

#endif
