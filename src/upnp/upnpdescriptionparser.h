/*
    SPDX-FileCopyrightText: 2005-2007 Joris Guisson
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
#ifndef KTUPNPDESCRIPTIONPARSER_H
#define KTUPNPDESCRIPTIONPARSER_H

#include <QString>

namespace bt
{
class UPnPRouter;

/**
 * @author Joris Guisson
 *
 * Parses the xml description of a router.
 */
class UPnPDescriptionParser
{
public:
    UPnPDescriptionParser();
    virtual ~UPnPDescriptionParser();

    /**
     * Parse the xml description.
     * @param file File it is located in
     * @param router The router off the xml description
     * @return true upon success
     */
    bool parse(const QString &file, UPnPRouter *router);

    /**
     * Parse the xml description.
     * @param data QByteArray with the data
     * @param router The router off the xml description
     * @return true upon success
     */
    bool parse(const QByteArray &data, UPnPRouter *router);
};

}

#endif
