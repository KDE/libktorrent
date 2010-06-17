/***************************************************************************
 *   Copyright (C) 2005-2007 by Joris Guisson                              *
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
#ifndef KTUPNPROUTER_H
#define KTUPNPROUTER_H

#include <kurl.h>
#include <qstringlist.h>
#include <net/portlist.h>
#include <ktorrent_export.h>

class KJob;

namespace bt
{
	class HTTPRequest;
	class WaitJob;

	/**
	 * Structure describing a UPnP service found in an xml file.
	*/
	struct KTORRENT_EXPORT UPnPService	
	{ 
		QString serviceid;
		QString servicetype;
		QString controlurl;
		QString eventsuburl;
		QString scpdurl;
		
		UPnPService();
		UPnPService(const UPnPService & s);
		
		/**
		 * Set a property of the service.
		 * @param name Name of the property (matches to variable names)
		 * @param value Value of the property
		 */
		void setProperty(const QString & name,const QString & value);
		
		/**
		 * Set all strings to empty.
		 */
		void clear();
		
		/**
		 * Assignment operator
		 * @param s The service to copy
		 * @return *this
		 */
		UPnPService & operator = (const UPnPService & s);
	};
	
	/**
	 *  Struct to hold the description of a device
	 */
	struct KTORRENT_EXPORT UPnPDeviceDescription
	{
		QString friendlyName;
		QString manufacturer;
		QString modelDescription;
		QString modelName;
		QString modelNumber;
		
		/**
		 * Set a property of the description
		 * @param name Name of the property (matches to variable names)
		 * @param value Value of the property
		 */
		void setProperty(const QString & name,const QString & value);
	};
	
	/**
	 * @author Joris Guisson
	 * 
	 * Class representing a UPnP enabled router. This class is also used to communicate
	 * with the router.
	*/
	class KTORRENT_EXPORT UPnPRouter : public QObject
	{
		Q_OBJECT
	public:
		/**
		 * Construct a router.
		 * @param server The name of the router
		 * @param location The location of it's xml description file
		 * @param verbose Print lots of debug info
		 */
		UPnPRouter(const QString & server,const KUrl & location,bool verbose = false);	
		virtual ~UPnPRouter();

		/// Disable or enable verbose logging
		void setVerbose(bool v);
		
		/// Get the name  of the server
		QString getServer() const;
		
		/// Get the location of it's xml description
		KUrl getLocation() const;
		
		/// Get the device description
		UPnPDeviceDescription & getDescription();
		
		/// Get the device description (const version)
		const UPnPDeviceDescription & getDescription() const;
		
		/// Get the current error (null string if there is none)
		QString getError() const;
		
		/**
		 * Download the XML File of the router.
		 */
		void downloadXMLFile();
		
		/**
		 * Add a service to the router.
		 * @param s The service
		 */
		void addService(const UPnPService & s);
			
#if 0
		/**
		 * See if a port is forwarded
		 * @param port The Port
		 */
		void isPortForwarded(const net::Port & port);	
		
		/**
		 * Get the external IP address.
		 */
		void getExternalIP();
#endif
		
		/**
		 * Forward a local port
		 * @param port The local port to forward
		 */
		void forward(const net::Port & port);
		
		/**
		 * Undo forwarding
		 * @param port The port
		 * @param waitjob When this is set the jobs needs to be added to the waitjob, 
		 * so we can wait for their completeion at exit
		 */
		void undoForward(const net::Port & port,bt::WaitJob* waitjob = 0);
		
		/**
			In order to iterate over all forwardings, the visitor pattern must be used.
		*/
		class Visitor
		{
		public:
			virtual ~Visitor() {}
			
			/**
				Called for each forwarding.
				@param port The Port
				@param pending When set to true, the forwarding is not completed yet
				@param service The UPnPService this forwarding is for
			*/
			virtual void forwarding(const net::Port & port, bool pending, const UPnPService* service) = 0;
		};
		
		/**
			Visit all forwardings
			@param visitor The Visitor
		*/
		void visit(Visitor* visitor) const;
	
		
	private slots:
		void onReplyOK(HTTPRequest* r,const QString &);
		void onReplyError(HTTPRequest* r,const QString &);
		void onError(HTTPRequest* r,const QString & msg);
		void downloadFinished(KJob* j);
		
	signals:
		/**
		 * Internal state has changed, a forwarding succeeded or failed, or an undo forwarding succeeded or failed.
		 */
		void stateChanged();
		
		/**
		 * Signal which indicates that the XML was downloaded successfully or not.
		 * @param r The router which emitted the signal
		 * @param success Whether or not it succeeded
		 */
		void xmlFileDownloaded(UPnPRouter* r,bool success);
		
	private:
		class UPnPRouterPrivate;
		UPnPRouterPrivate* d;
	};

}

#endif
