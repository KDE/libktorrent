/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "logsystemmanager.h"
#include "log.h"
#include <KLocalizedString>

namespace bt
{
QScopedPointer<LogSystemManager> LogSystemManager::self;

LogSystemManager::LogSystemManager()
    : QObject()
{
    // register default systems
    registerSystem(i18n("General"), SYS_GEN);
    registerSystem(i18n("Connections"), SYS_CON);
    registerSystem(i18n("Tracker"), SYS_TRK);
    registerSystem(i18n("DHT"), SYS_DHT);
    registerSystem(i18n("Disk Input/Output"), SYS_DIO);
    registerSystem(i18n("µTP"), SYS_UTP);
}

LogSystemManager::~LogSystemManager()
{
}

LogSystemManager &LogSystemManager::instance()
{
    if (!self) {
        self.reset(new LogSystemManager());
    }
    return *self;
}

void LogSystemManager::registerSystem(const QString &name, Uint32 id)
{
    systems.insert(name, id);
    Q_EMIT registered(name);
}

void LogSystemManager::unregisterSystem(const QString &name)
{
    if (systems.remove(name)) {
        Q_EMIT unregisted(name);
    }
}

Uint32 LogSystemManager::systemID(const QString &name)
{
    iterator i = systems.find(name);
    if (i == systems.end()) {
        return 0;
    } else {
        return i.value();
    }
}

}

#include "moc_logsystemmanager.cpp"
