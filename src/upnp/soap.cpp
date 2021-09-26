/*
    SPDX-FileCopyrightText: 2005-2007 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
