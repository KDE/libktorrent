/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_TICKETMANAGER_H
#define BT_TICKETMANAGER_H

#include "constants.h"
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <ktorrent_export.h>

namespace bt
{
class ResourceManager;

/*!
    \brief Represents a scarce resource which must be acquired from a ResourceManager.

    The ResourceManager will notify the Resource when it has been acquired.
 */
class KTORRENT_EXPORT Resource
{
public:
    Resource(ResourceManager *rman, const QString &group);
    virtual ~Resource();

    //! Get the name of the group the resource is part of
    QString groupName() const
    {
        return group;
    }

    //! Called when the resource has been acquired
    virtual void acquired() = 0;

    //! Release the Resource
    void release();

    typedef QSet<Resource *> Set;
    typedef QList<Resource *> List;

private:
    ResourceManager *rman;
    QString group;
};

/*!
 * \brief Distributes resources equally over several groups, ensuring that each group gets its fair share.
 */
class KTORRENT_EXPORT ResourceManager
{
public:
    ResourceManager(Uint32 max_active_resources);
    virtual ~ResourceManager();

    /*!
        Set max active resources
     */
    void setMaxActive(Uint32 m)
    {
        max_active_resources = m;
    }

    /*!
        Add a Resource
        \param r The Resource
     */
    void add(Resource *r);

    /*!
     * Try to have a Resource acquired. If it succeeds, it will be added and
     * true will be returned. If it fails nothing will happen and false is returned.
     * \param r The Resource
     * \return true upon success, false otherwise
     */
    bool acquire(Resource *r);

    /*!
        Remove a resource.
        \param r The Resource
     */
    void remove(Resource *r);

    /*!
     * Shutdown the resource manager, no more resources will be handed out.
     */
    void shutdown();

private:
    void update();

private:
    Uint32 max_active_resources;
    Resource::Set active;
    QMap<QString, Resource::List> pending;
    QString current;
};

}

#endif // BT_TICKETMANAGER_H
