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

#include "upnpdescriptionparser.h"

#include <QFile>
#include <QStack>
#include <QStringRef>
#include <QXmlStreamReader>

#include <util/fileops.h>
#include <util/log.h>
#include "upnprouter.h"

using namespace bt;

namespace bt
{
	
	class XMLContentHandler
	{
		enum Status
		{
		    TOPLEVEL,ROOT,DEVICE,SERVICE,FIELD,OTHER
		};

		QString tmp;
		UPnPRouter* router;
		UPnPService curr_service;
		QStack<Status> status_stack;
	public:
		XMLContentHandler(UPnPRouter* router);
		~XMLContentHandler();

		bool parse(const QByteArray& data);

		bool startDocument();
		bool endDocument();
		bool startElement(const QStringRef& namespaceUri, const QStringRef& localName,
                              const QStringRef& qName, const QXmlStreamAttributes& atts);
		bool endElement(const QStringRef& namespaceUri, const QStringRef& localName,
                            const QStringRef& qName);
		bool characters(const QStringRef& chars);
		
		bool interestingDeviceField(const QStringRef& name);
		bool interestingServiceField(const QStringRef& name);
	};


	UPnPDescriptionParser::UPnPDescriptionParser()
	{}


	UPnPDescriptionParser::~UPnPDescriptionParser()
	{}

	bool UPnPDescriptionParser::parse(const QString & file, UPnPRouter* router)
	{
		QFile fptr(file);
		if (!fptr.open(QIODevice::ReadOnly))
			return false;

		QByteArray data = fptr.readAll();
		XMLContentHandler chandler(router);

		const bool ret = chandler.parse(data);

		if (!ret)
		{
			Out(SYS_PNP|LOG_IMPORTANT) << "Error parsing XML" << endl;
			return false;
		}
		return true;
	}
	
	bool UPnPDescriptionParser::parse(const QByteArray & data, UPnPRouter* router)
	{
		XMLContentHandler chandler(router);

		const bool ret = chandler.parse(data);

		
		if (!ret)
		{
			Out(SYS_PNP|LOG_IMPORTANT) << "Error parsing XML" << endl;
			return false;
		}
		return true;
	}

	/////////////////////////////////////////////////////////////////////////////////


	XMLContentHandler::XMLContentHandler(UPnPRouter* router) : router(router)
	{}

	XMLContentHandler::~XMLContentHandler()
	{}

	bool XMLContentHandler::parse(const QByteArray& data)
	{
		QXmlStreamReader reader(data);

		while (!reader.atEnd()) {
			reader.readNext();
			if (reader.hasError())
				return false;

			switch (reader.tokenType()) {
			case QXmlStreamReader::StartDocument:
				if (!startDocument()) {
					return false;
				}
				break;
			case QXmlStreamReader::EndDocument:
				if (!endDocument()) {
					return false;
				}
				break;
			case QXmlStreamReader::StartElement:
				if (!startElement(reader.namespaceUri(), reader.name(),
						  reader.qualifiedName(), reader.attributes())) {
					return false;
				}
				break;
			case QXmlStreamReader::EndElement:
				if (!endElement(reader.namespaceUri(), reader.name(),
						reader.qualifiedName())) {
					return false;
				}
				break;
			case QXmlStreamReader::Characters:
				if (!reader.isWhitespace() && !reader.text().trimmed().isEmpty()) {
					if (!characters(reader.text()))
						return false;
				}
				break;
			default:
				break;
			}
		}

		if (!reader.isEndDocument())
			return false;

		return true;
	}

	bool XMLContentHandler::startDocument()
	{
		status_stack.push(TOPLEVEL);
		return true;
	}

	bool XMLContentHandler::endDocument()
	{
		status_stack.pop();
		return true;
	}
	
	bool XMLContentHandler::interestingDeviceField(const QStringRef& name)
	{
		return name == "friendlyName" || name == "manufacturer" || name == "modelDescription" ||
				name == "modelName" || name == "modelNumber";
	}

	
	bool XMLContentHandler::interestingServiceField(const QStringRef& name)
	{
		return name == "serviceType" || name == "serviceId" || name == "SCPDURL" ||
				name == "controlURL" || name == "eventSubURL";
	}

	bool XMLContentHandler::startElement(const QStringRef& namespaceUri, const QStringRef& localName,
					     const QStringRef& qName, const QXmlStreamAttributes& atts)
	{
		Q_UNUSED(namespaceUri)
		Q_UNUSED(qName)
		Q_UNUSED(atts)

		tmp = "";
		switch (status_stack.top())
		{
		case TOPLEVEL:
			// from toplevel we can only go to root
			if (localName == "root")
				status_stack.push(ROOT);
			else
				return false;
			break;
		case ROOT:
			// from the root we can go to device or specVersion
			// we are not interested in the specVersion
			if (localName == "device")
				status_stack.push(DEVICE);
			else
				status_stack.push(OTHER);
			break;
		case DEVICE:
			// see if it is a field we are interested in
			if (interestingDeviceField(localName))
				status_stack.push(FIELD);
			else
				status_stack.push(OTHER);
			break;
		case SERVICE:
			if (interestingServiceField(localName))
				status_stack.push(FIELD);
			else
				status_stack.push(OTHER);
			break;
		case OTHER:
			if (localName == "service")
				status_stack.push(SERVICE);
			else if (localName == "device")
				status_stack.push(DEVICE);
			else
				status_stack.push(OTHER);
			break;
		case FIELD:
			break;
		}
		return true;
	}

	bool XMLContentHandler::endElement(const QStringRef& namespaceUri, const QStringRef& localName,
					   const QStringRef& qName)
	{
		Q_UNUSED(namespaceUri)
		Q_UNUSED(qName)

		switch (status_stack.top())
		{
		case FIELD:
			// we have a field so set it
			status_stack.pop();
			if (status_stack.top() == DEVICE)
			{
				// if we are in a device
				router->getDescription().setProperty(localName.toString(), tmp);
			}
			else if (status_stack.top() == SERVICE)
			{
				// set a property of a service
				curr_service.setProperty(localName.toString(), tmp);
			}
			break;
		case SERVICE:
			// add the service
			router->addService(curr_service);
			curr_service.clear();
			// pop the stack
			status_stack.pop();
			break;
		default:
			status_stack.pop();
			break;
		}

		// reset tmp
		tmp = "";
		return true;
	}

	bool XMLContentHandler::characters(const QStringRef& chars)
	{
		tmp.append(chars);
		return true;
	}

}
