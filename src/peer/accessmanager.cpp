/***************************************************************************
 *   Copyright (C) 2008 by Joris Guisson and Ivan Vasic                    *
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
#include "accessmanager.h"

#include <net/address.h>
#include <peer/badpeerslist.h>
#include <interfaces/blocklistinterface.h>
#include <tracker/tracker.h>
#include <torrent/server.h>

namespace bt
{

    AccessManager::AccessManager()
    {
        banned = new BadPeersList();
        addBlockList(banned);
    }


    AccessManager::~AccessManager()
    {
        qDeleteAll(blocklists);
    }

    AccessManager& AccessManager::instance()
    {
        static AccessManager inst;
        return inst;
    }

    void AccessManager::addBlockList(BlockListInterface* bl)
    {
        blocklists.append(bl);
    }

    void AccessManager::removeBlockList(BlockListInterface* bl)
    {
        blocklists.removeAll(bl);
    }

    bool AccessManager::allowed(const net::Address& addr) const
    {
        // Don't connect to the custom IP sent to the tracker, this should be our own'
        if(isOurOwnAddress(addr))
            return false;

        foreach(const BlockListInterface* bl, blocklists)
        {
            if(bl->blocked(addr))
                return false;
        }
        return true;
    }

    void AccessManager::banPeer(const QString& addr)
    {
        banned->addBadPeer(addr);
    }
    
    void AccessManager::addExternalIP(const QString& addr)
    {
        external_addresses.append(addr);
    }
    
    bool AccessManager::isOurOwnAddress(const net::Address& addr) const
    {
        Uint16 port = bt::Server::getPort();
        if(!Tracker::getCustomIP().isEmpty() && net::Address(Tracker::getCustomIP(), port) == addr)
            return true;
            
           
        foreach(const QString& ip, external_addresses)
        {
            net::Address address(ip, port);
            if(address == addr)
                return true;
        }
        
        return false;
    }

}
