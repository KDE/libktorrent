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

#include "soap.h"

namespace bt
{
QString SOAP::createCommand(const QString &action, const QString &service)
{
    QString comm = QString(
                       "<?xml version=\"1.0\"?>"
                       "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                       "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                       "<SOAP-ENV:Body>"
                       "<m:%1 xmlns:m=\"%2\"/>"
                       "</SOAP-ENV:Body></SOAP-ENV:Envelope>")
                       .arg(action)
                       .arg(service);

    return comm;
}

QString SOAP::createCommand(const QString &action, const QString &service, const QList<Arg> &args)
{
    QString comm = QString(
                       "<?xml version=\"1.0\"?>"
                       "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                       "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                       "<SOAP-ENV:Body>"
                       "<m:%1 xmlns:m=\"%2\">")
                       .arg(action)
                       .arg(service);

    for (const Arg &a : args)
        comm += "<" + a.element + ">" + a.value + "</" + a.element + ">";

    comm += QString("</m:%1></SOAP-ENV:Body></SOAP-ENV:Envelope>").arg(action);
    return comm;
}
}
