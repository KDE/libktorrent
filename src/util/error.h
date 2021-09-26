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
#ifndef BTERROR_H
#define BTERROR_H

#include <ktorrent_export.h>
#include <qstring.h>

namespace bt
{
/**
    @author Joris Guisson
*/
class KTORRENT_EXPORT Error
{
    QString msg;

public:
    Error(const QString &msg);
    virtual ~Error();

    QString toString() const
    {
        return msg;
    }
};

class KTORRENT_EXPORT Warning
{
    QString msg;

public:
    Warning(const QString &msg);
    virtual ~Warning();

    QString toString() const
    {
        return msg;
    }
};
}

#endif
