/*
    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "soap.h"

using namespace Qt::Literals::StringLiterals;

namespace bt
{
QString SOAP::createCommand(const QString &action, const QString &service)
{
    QString comm = QStringLiteral(
                       "<?xml version=\"1.0\"?>"
                       "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                       "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                       "<SOAP-ENV:Body>"
                       "<m:%1 xmlns:m=\"%2\"/>"
                       "</SOAP-ENV:Body></SOAP-ENV:Envelope>")
                       .arg(action, service);

    return comm;
}

QString SOAP::createCommand(const QString &action, const QString &service, const QList<Arg> &args)
{
    QString comm = QStringLiteral(
                       "<?xml version=\"1.0\"?>"
                       "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                       "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                       "<SOAP-ENV:Body>"
                       "<m:%1 xmlns:m=\"%2\">")
                       .arg(action, service);

    for (const Arg &a : args) {
        comm += '<'_L1 + a.element + '>'_L1 + a.value + "</"_L1 + a.element + '>'_L1;
    }

    comm += u"</m:%1></SOAP-ENV:Body></SOAP-ENV:Envelope>"_s.arg(action);
    return comm;
}
}
