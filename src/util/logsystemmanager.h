/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTLOGSYSTEMMANAGER_H
#define BTLOGSYSTEMMANAGER_H

#include <QMap>
#include <QObject>
#include <QScopedPointer>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
/*!
    \brief Keeps track of all logging system ID's.
*/
class KTORRENT_EXPORT LogSystemManager : public QObject
{
    Q_OBJECT

    LogSystemManager();

public:
    ~LogSystemManager() override;

    //! Register a system
    void registerSystem(const QString &name, Uint32 id);

    //! Unregister a system
    void unregisterSystem(const QString &name);

    using iterator = QMap<QString, Uint32>::iterator;
    using const_iterator = QMap<QString, Uint32>::const_iterator;

    iterator begin()
    {
        return systems.begin();
    }
    iterator end()
    {
        return systems.end();
    }

    [[nodiscard]] const_iterator begin() const
    {
        return systems.cbegin();
    }
    [[nodiscard]] const_iterator end() const
    {
        return systems.cend();
    }

    static LogSystemManager &instance();

    //! Get the ID of a system
    Uint32 systemID(const QString &name);

Q_SIGNALS:
    void registered(const QString &name);
    void unregisted(const QString &name);

private:
    QMap<QString, Uint32> systems;
    static QScopedPointer<LogSystemManager> self;
};

}

#endif
