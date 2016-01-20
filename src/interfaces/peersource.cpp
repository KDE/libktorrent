/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
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
#include "peersource.h"

namespace bt
{

	PeerSource::PeerSource() 
	{}


	PeerSource::~PeerSource()
	{}
	
	void PeerSource::completed()
	{}
	
	void PeerSource::manualUpdate()
	{}
	
	void PeerSource::aboutToBeDestroyed()
	{}
	
	void PeerSource::addPeer(const net::Address & addr, bool local)
	{
		peers.append(qMakePair(addr, local));
	}
	
	bool PeerSource::takePeer(net::Address& addr, bool& local)
	{
		if (peers.count() > 0)
		{
			addr = peers.front().first;
			local = peers.front().second;
			peers.pop_front();
			return true;
		}
		return false;
	}

}
