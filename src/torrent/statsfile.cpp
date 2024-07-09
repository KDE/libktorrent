/*
    SPDX-FileCopyrightText: 2005-2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "statsfile.h"

#include <KConfigGroup>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
StatsFile::StatsFile(const QString &filename)
    : cfg(KSharedConfig::openConfig(filename))
{
}

StatsFile::~StatsFile()
{
}

void StatsFile::write(const QString &key, const QString &value)
{
    cfg->group(QString()).writeEntry(key, value);
}

QString StatsFile::readString(const QString &key)
{
    KConfigGroup g = cfg->group(QString());
    return g.readEntry(key).trimmed();
}

Uint64 StatsFile::readUint64(const QString &key)
{
    bool ok = true;
    Uint64 val = readString(key).toULongLong(&ok);
    return val;
}

int StatsFile::readInt(const QString &key)
{
    bool ok = true;
    int val = readString(key).toInt(&ok);
    return val;
}

bool StatsFile::readBoolean(const QString &key)
{
    return (bool)readInt(key);
}

unsigned long StatsFile::readULong(const QString &key)
{
    bool ok = true;
    return readString(key).toULong(&ok);
}

float StatsFile::readFloat(const QString &key)
{
    bool ok = true;
    return readString(key).toFloat(&ok);
}

void StatsFile::sync()
{
    cfg->sync();
}

bool StatsFile::hasKey(const QString &key) const
{
    return cfg->group(QString()).hasKey(key);
}

}
