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
#ifndef BTLOGMONITORINTERFACE_H
#define BTLOGMONITORINTERFACE_H

#include <ktorrent_export.h>

class QString;

namespace bt
{
/**
 * @author Joris Guisson
 * @brief Interface for classes who which to receive which log messages are printed
 *
 * This class is an interface for all classes which want to know,
 * what is written to the log.
 */
class KTORRENT_EXPORT LogMonitorInterface
{
public:
    LogMonitorInterface();
    virtual ~LogMonitorInterface();

    /**
     * A line was written to the log file.
     * @param line The line
     */
    virtual void message(const QString &line, unsigned int arg) = 0;
};

}

#endif
