/***************************************************************************
 *   Copyright (C) 2007 by Joris Guisson and Ivan Vasic                    *
 *   joris.guisson@gmail.com                                               *
 *   ivasic@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "version.h"
#include <QString>


namespace bt
{
static QString g_name = QStringLiteral("KTorrent");
static int g_major = 0;
static int g_minor = 0;
static int g_release = 0;
static QString g_peer_id = QStringLiteral("KT");

// TODO: create a function SetClientInfo(const QString &name, QString &version, const QString &peer_id) and  mark this one [[deprecated]]
void SetClientInfo(const QString & name, int major, int minor, int release, VersionType type, const QString & peer_id)
{
    Q_UNUSED(type)
    g_name = name;
    g_major = major;
    g_minor = minor;
    g_release = release;
    g_peer_id = peer_id;
}

QString PeerIDPrefix()
{
    return QString("-%1%2%3%4-").arg(g_peer_id).arg(g_major).arg(g_minor).arg(g_release);
}

QString GetVersionString()
{
    QString str = g_name + QString("/%1.%2.%3").arg(g_major).arg(g_minor).arg(g_release);
    return str;
}
}
