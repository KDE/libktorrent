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
#ifndef DHTRPCCALL_H
#define DHTRPCCALL_H

#include <qtimer.h>
#include "key.h"
#include "rpcmsg.h"

namespace dht
{
	class RPCCall;

	/**
	 * Class which objects should derive from, if they want to know the result of a call.
	*/
	class RPCCallListener : public QObject
	{
		Q_OBJECT
	public:
		RPCCallListener(QObject* parent);
		virtual ~RPCCallListener();

	public Q_SLOTS:
		/**
		 * A response was received.
		 * @param c The call
		 * @param rsp The response
		 */
		virtual void onResponse(RPCCall* c, RPCMsg::Ptr rsp) = 0;

		/**
		 * The call has timed out.
		 * @param c The call
		 */
		virtual void onTimeout(RPCCall* c) = 0;
	};

	/**
	 * @author Joris Guisson
	 */
	class RPCCall : public QObject
	{
		Q_OBJECT
	public:
		RPCCall(RPCMsg::Ptr msg, bool queued);
		virtual ~RPCCall();

		/**
		 * Called when a queued call gets started. Starts the timeout timer.
		 */
		void start();

		/**
		 * Called by the server if a response is received.
		 * @param rsp
		 */
		void response(RPCMsg::Ptr rsp);

		/**
		 * Add a listener for this call
		 * @param cl The listener
		 */
		void addListener(RPCCallListener* cl);

		/// Get the message type
		Method getMsgMethod() const;

		/// Get the request sent
		const RPCMsg::Ptr getRequest() const {return msg;}

		/// Get the request sent
		RPCMsg::Ptr getRequest() {return msg;}

	private Q_SLOTS:
		void onTimeout();

	Q_SIGNALS:
		void response(RPCCall* c, RPCMsg::Ptr rsp);
		void timeout(RPCCall* c);

	private:
		RPCMsg::Ptr msg;
		QTimer timer;
		bool queued;
	};

}

#endif
