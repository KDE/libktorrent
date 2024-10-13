/*
    SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "version.h"
#include <QString>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
static QString g_name = QStringLiteral("KTorrent");
static QString g_version = QStringLiteral("0.0.0");
static QString g_peer_id = QStringLiteral("KT");
static QString g_version_without_dots = QStringLiteral("000");

void SetClientInfo(const QString &name, const QString &version, const QString &peer_id)
{
    g_name = name;
    g_version = version;
    g_peer_id = peer_id;
    g_version_without_dots = g_version;
    g_version_without_dots = g_version_without_dots.remove('.'_L1);
}

[[deprecated]] void SetClientInfo(const QString &name, int major, int minor, int release, VersionType type, const QString &peer_id)
{
    Q_UNUSED(type)
    g_name = name;
    g_version = u"%1.%2.%3"_s.arg(major).arg(minor).arg(release);
    g_peer_id = peer_id;
    g_version_without_dots = u"%1%2%3"_s.arg(major).arg(minor).arg(release);
}

QString PeerIDPrefix()
{
    return u"-%1%2-"_s.arg(g_peer_id, g_version_without_dots);
}

QString GetVersionString()
{
    QString str = g_name + '/'_L1 + g_version;
    return str;
}
}
