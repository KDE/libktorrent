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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#ifndef DHTRPCSERVER_H
#define DHTRPCSERVER_H

#include <QList>
#include <QObject>
#include <dht/rpcserverinterface.h>
#include <net/address.h>
#include <net/socket.h>
#include <util/constants.h>
#include <util/ptrmap.h>

using bt::Uint32;
using bt::Uint16;
using bt::Uint8;

namespace dht
{
	class Key;
	class DHT;

	/**
	 * @author Joris Guisson
	 *
	 * Class to handle incoming and outgoing RPC messages.
	 */
	class RPCServer : public QObject, public RPCServerInterface
	{
		Q_OBJECT
	public:
		RPCServer(DHT* dh_table, Uint16 port, QObject *parent = 0);
		virtual ~RPCServer();

		/// Start the server
		void start();

		/// Stop the server
		void stop();

		/**
		 * Do a RPC call.
		 * @param msg The message to send
		 * @return The call object
		 */
		virtual RPCCall* doCall(RPCMsg::Ptr msg);

		/**
		 * Send a message, this only sends the message, it does not keep any call
		 * information. This should be used for replies.
		 * @param msg The message to send
		 */
		void sendMsg(RPCMsg::Ptr msg);

		/**
		 * Send a message, this only sends the message, it does not keep any call
		 * information. This should be used for replies.
		 * @param msg The message to send
		 */
		void sendMsg(const RPCMsg & msg);

		/**
		 * Ping a node, we don't care about the MTID.
		 * @param addr The address
		 */
		void ping(const dht::Key & our_id, const net::Address & addr);

		/// Get the number of active calls
		Uint32 getNumActiveRPCCalls() const;
		
	private slots:
		void callTimeout(RPCCall* call);

	private:
		class Private;
		Private* d;
	};

}

#endif
