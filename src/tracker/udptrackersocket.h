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
#ifndef BTUDPTRACKERSOCKET_H
#define BTUDPTRACKERSOCKET_H

#include <QObject>
#include <QByteArray>
#include <util/constants.h>
#include <util/bufferpool.h>
#include <ktorrent_export.h>

#ifdef ERROR
#undef ERROR
#endif

namespace net
{
	class Address;
}

namespace bt
{

	/**
	 * @author Joris Guisson
	 *
	 * Class which handles communication with one or more UDP trackers.
	*/
	class KTORRENT_EXPORT UDPTrackerSocket : public QObject
	{
		Q_OBJECT
	public:
		UDPTrackerSocket();
		virtual ~UDPTrackerSocket();
		
		enum Action
		{
			CONNECT = 0,
			ANNOUNCE = 1,
			SCRAPE = 2,
			ERROR = 3
		};

		/**
		 * Send a connect message. As a response to this, the connectReceived
		 * signal will be emitted, classes recieving this signal should check if
		 * the transaction_id is the same.
		 * @param tid The transaction_id 
		 * @param addr The address to send to
		 */
		void sendConnect(Int32 tid,const net::Address & addr);

		/**
		 * Send an announce message. As a response to this, the announceReceived
		 * signal will be emitted, classes recieving this signal should check if
		 * the transaction_id is the same.
		 * @param tid The transaction_id
		 * @param data The data to send (connect input structure, in UDP Tracker specifaction)
		 * @param addr The address to send to
		 */
		void sendAnnounce(Int32 tid,const Uint8* data,const net::Address & addr);
		
		/**
		* Send a scrape message. As a response to this, the scrapeReceived
		* signal will be emitted, classes recieving this signal should check if
		* the transaction_id is the same.
		* @param tid The transaction_id
		* @param data The data to send (connect input structure, in UDP Tracker specifaction)
		* @param addr The address to send to
		*/
		void sendScrape(Int32 tid,const Uint8* data,const net::Address & addr);

		/**
		 * If a transaction times out, this should be used to cancel it.
		 * @param tid 
		 */
		void cancelTransaction(Int32 tid);


		/**
		 * Compute a free transaction_id.
		 * @return A free transaction_id
		 */
		Int32 newTransactionID();
		
		/**
		 * Set the port ot use.
		 * @param p The port
		 */
		static void setPort(Uint16 p);
		
		/// Get the port in use.
		static Uint16 getPort();

	signals:
		/**
		 * Emitted when a connect message is received.
		 * @param tid The transaction_id
		 * @param connection_id The connection_id returned
		 */
		void connectReceived(Int32 tid,Int64 connection_id);
		
		/**
		 * Emitted when an announce message is received.
		 * @param tid The transaction_id
		 * @param buf The data
		 * @param size The data size
		 */
		void announceReceived(Int32 tid,const Uint8* buf,Uint32 size);
		
		/**
		* Emitted when a scrape message is received.
		* @param tid The transaction_id
		* @param buf The data
		* @param size The data size
		*/
		void scrapeReceived(Int32 tid,const Uint8* buf,Uint32 size);

		/**
		 * Signal emitted, when an error occurs during a transaction.
		 * @param tid The transaction_id
		 * @param error_string Potential error string
		 */
		void error(Int32 tid,const QString & error_string);

	private:
		void handleConnect(bt::Buffer::Ptr buf);
		void handleAnnounce(bt::Buffer::Ptr buf);
		void handleError(bt::Buffer::Ptr buf);
		void handleScrape(bt::Buffer::Ptr buf);
		
	private:
		class Private;
		Private* d;
		
		static Uint16 port;
	};
}

#endif
