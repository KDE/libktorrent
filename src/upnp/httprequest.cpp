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

#include "httprequest.h"
#include <QHostAddress>
#include <QNetworkReply>
#include <QStringList>
#include <QTimer>

#include <KLocalizedString>

#include <util/log.h>

namespace bt
{
HTTPRequest::HTTPRequest(const QNetworkRequest &hdr, const QString &payload, const QString &host, Uint16 port, bool verbose)
    : hdr(hdr)
    , m_payload(payload)
    , verbose(verbose)
    , host(host)
    , port(port)
    , success(false)
{
    networkAccessManager = new QNetworkAccessManager(this);
    networkAccessManager->connectToHost(host, port);

    QTcpSocket socket;
    QString localAddress;
    socket.connectToHost(host, port);
    if (socket.waitForConnected()) {
        localAddress = socket.localAddress().toString();
        socket.close();
    } else {
        Out(SYS_PNP | LOG_DEBUG) << "TCP connection timeout" << endl;
        socket.close();
        error = i18n("Operation timed out");
        success = false;
        Q_EMIT result(this);
        operationFinished(this);
        return;
    }

    m_payload = m_payload.replace(QLatin1String("$LOCAL_IP"), localAddress);
}

void HTTPRequest::start()
{
    networkReply = networkAccessManager->post(hdr, m_payload.toLatin1());
    connect(networkReply, &QNetworkReply::finished, this, &HTTPRequest::replyFinished);
}

void HTTPRequest::replyFinished()
{
    if (networkReply->error()) {
        error = networkReply->errorString();
        success = false;
        Q_EMIT result(this);
        operationFinished(this);
        return;
    }
    reply = networkReply->readAll();
    networkReply->deleteLater();
    success = true;
    Q_EMIT result(this);
    operationFinished(this);
}

}
