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

#include "peerconnector.h"
#include <QSet>
#include <interfaces/serverinterface.h>
#include <mse/encryptedauthenticate.h>
#include <torrent/torrent.h>
#include "peermanager.h"
#include "authenticationmonitor.h"


namespace bt
{
	static ResourceManager half_open_connections(50);
	
	class PeerConnector::Private
	{
	public:
		Private(PeerConnector* p,const QString & ip,Uint16 port,bool local,PeerManager* pman)
		: p(p),ip(ip),port(port),local(local),pman(pman),auth(0),stopping(false),do_not_start(false)
		{
		}
		
		~Private()
		{
			if (auth)
			{
				stopping = true;
				auth->stop();
				stopping = false;
			}
		}
		
		void start(Method method);
		void stop();
		void authenticationFinished(Authenticate* auth, bool ok);
		
	public:
		PeerConnector* p;
		QSet<Method> tried_methods;
		Method current_method;
		QString ip;
		Uint16 port;
		bool local;
		QWeakPointer<PeerManager> pman;
		Authenticate* auth;
		bool stopping;
		bool do_not_start;
	};
	
	PeerConnector::PeerConnector(const QString& ip, Uint16 port, bool local, bt::PeerManager* pman) 
		: QObject(pman),
		Resource(&half_open_connections,pman->getTorrent().getInfoHash().toString()),
		d(new Private(this,ip,port,local,pman))
	{
	}

	PeerConnector::~PeerConnector()
	{
		delete d;
	}
	
	void PeerConnector::setMaxActive(Uint32 mc)
	{
		half_open_connections.setMaxActive(mc);
	}
	
	void PeerConnector::start()
	{
		half_open_connections.add(this);
	}
	
	void PeerConnector::acquired()
	{
		PeerManager* pm = d->pman.data();
		if (!pm || !pm->isStarted())
			return;
		
		bool encryption = ServerInterface::isEncryptionEnabled();
		bool utp = ServerInterface::isUtpEnabled();
		
		if (encryption)
		{
			if (utp)
				d->start(UTP_WITH_ENCRYPTION);
			else
				d->start(TCP_WITH_ENCRYPTION);
		}
		else
		{
			if (utp)
				d->start(UTP_WITHOUT_ENCRYPTION);
			else
				d->start(TCP_WITHOUT_ENCRYPTION);
		}
	}
	
	void PeerConnector::Private::stop()
	{
		if (auth)
		{
			// Add all methods to the tried_methods set, so the authentication will stop
			tried_methods.insert(UTP_WITH_ENCRYPTION);
			tried_methods.insert(TCP_WITH_ENCRYPTION);
			tried_methods.insert(UTP_WITHOUT_ENCRYPTION);
			tried_methods.insert(TCP_WITHOUT_ENCRYPTION);
			auth->stop();
		}
	}
	
	void PeerConnector::stop()
	{
		d->stop();
		release();
	}



	void PeerConnector::authenticationFinished(Authenticate* auth, bool ok)
	{
		d->authenticationFinished(auth,ok);
	}
	
	void PeerConnector::Private::authenticationFinished(Authenticate* auth, bool ok)
	{
		this->auth = 0;
		if (stopping)
			return;
		
		PeerManager* pm = pman.data();
		if (!pm)
			return;
		
		if (ok)
		{
			pm->peerAuthenticated(auth,p,ok);
			return;
		}
		
		tried_methods.insert(current_method);
		
		QList<Method> allowed;
		
		bool encryption = ServerInterface::isEncryptionEnabled();
		bool only_use_encryption = !ServerInterface::unencryptedConnectionsAllowed();
		bool utp = ServerInterface::isUtpEnabled();
		bool only_use_utp = ServerInterface::onlyUseUtp();
		
		if (utp && encryption)
			allowed.append(UTP_WITH_ENCRYPTION);
		if (!only_use_utp && encryption)
			allowed.append(TCP_WITH_ENCRYPTION);
		if (utp && !only_use_encryption)
			allowed.append(UTP_WITHOUT_ENCRYPTION);
		if (!only_use_utp && !only_use_encryption)
			allowed.append(TCP_WITHOUT_ENCRYPTION);
		
		foreach (Method m,tried_methods)
			allowed.removeAll(m);
		
		if (allowed.isEmpty())
			pm->peerAuthenticated(auth,p,false);
		else
			start(allowed.front());
	}

	void PeerConnector::Private::start(PeerConnector::Method method)
	{
		PeerManager* pm = pman.data();
		if (!pm)
			return;
		
		current_method = method;
		const Torrent & tor = pm->getTorrent();
		TransportProtocol proto = (method == TCP_WITH_ENCRYPTION || method == TCP_WITHOUT_ENCRYPTION) ? TCP : UTP;
		if (method == TCP_WITH_ENCRYPTION || method == UTP_WITH_ENCRYPTION)
			auth = new mse::EncryptedAuthenticate(ip,port,proto,tor.getInfoHash(),tor.getPeerID(),p);
		else
			auth = new Authenticate(ip,port,proto,tor.getInfoHash(),tor.getPeerID(),p);
		
		if (local)
			auth->setLocal(true);
		
		AuthenticationMonitor::instance().add(auth);
	}

}


