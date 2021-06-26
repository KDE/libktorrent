/***************************************************************************
 *   Copyright (C) 2010 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
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

#include "resourcemanager.h"

namespace bt
{
Resource::Resource(bt::ResourceManager *rman, const QString &group)
    : rman(rman)
    , group(group)
{
}

Resource::~Resource()
{
    if (rman)
        rman->remove(this);
}

void Resource::release()
{
    if (rman) {
        rman->remove(this);
        rman = nullptr;
    }
}

ResourceManager::ResourceManager(Uint32 max_active_resources)
    : max_active_resources(max_active_resources)
{
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::add(Resource *r)
{
    QMap<QString, Resource::List>::iterator i = pending.find(r->groupName());
    if (i == pending.end())
        i = pending.insert(r->groupName(), Resource::List());

    i.value().append(r);
    if (current.isEmpty())
        current = r->groupName();
    update();
}

bool ResourceManager::acquire(Resource *r)
{
    if ((Uint32)active.size() < max_active_resources && pending.isEmpty()) {
        add(r);
        return true;
    } else
        return false;
}

void ResourceManager::remove(Resource *r)
{
    if (active.remove(r)) {
        update();
    } else {
        QMap<QString, Resource::List>::iterator i = pending.find(r->groupName());
        if (i != pending.end())
            i.value().removeAll(r);
    }
}

void ResourceManager::update()
{
    if ((Uint32)active.size() >= max_active_resources)
        return;

    QMap<QString, Resource::List>::iterator i = pending.find(current);
    if (i == pending.end())
        i = pending.begin();

    QMap<QString, Resource::List>::iterator start = i;
    Uint32 activated = 0;
    while ((Uint32)active.size() < max_active_resources) {
        if (!i.value().isEmpty()) {
            Resource *r = i.value().takeFirst();
            active.insert(r);
            r->acquired();
            activated++;
        }
        ++i;
        if (i == pending.end()) // Loop around
            i = pending.begin();

        if (i == start) { // we have completed an antire cycle
            if (activated == 0) // Nothing was activated, so exit the loop
                break;
            else
                activated = 0;
        }
    }

    current = i.key();
}

void ResourceManager::shutdown()
{
    max_active_resources = 0;
}

}
