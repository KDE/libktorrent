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

#ifndef KTHTTPREQUEST_H
#define KTHTTPREQUEST_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include <interfaces/exitoperation.h>
#include <util/constants.h>

namespace bt
{
/**
 * @author Joris Guisson
 *
 * Simple HTTP request class.
 * TODO: switch to KIO for this
 */
class HTTPRequest : public bt::ExitOperation
{
    Q_OBJECT
public:
    /**
     * Constructor, set the url and the request header.
     * @param hdr The http request header
     * @param payload The payload
     * @param host The host
     * @param port THe port
     * @param verbose Print traffic to the log
     */
    HTTPRequest(const QNetworkRequest &hdr, const QString &payload, const QString &host, bt::Uint16 port, bool verbose);
    /**
     * Open a connection and send the request.
     */
    void start();

    /**
        Get the reply data
    */
    QByteArray replyData() const
    {
        return reply;
    }

    /**
        Did the request succeed
    */
    bool succeeded() const
    {
        return success;
    }

    /**
        In case of failure this function will return an error string
    */
    QString errorString() const
    {
        return error;
    }

Q_SIGNALS:
    /**
     * An OK reply was sent.
     * @param r The sender of the request
     */
    void result(HTTPRequest *r);

public:
    void replyFinished(QNetworkReply *networkReply);

private:
    void parseReply(int eoh);

private:
    QNetworkRequest hdr;
    QString m_payload;
    bool verbose;
    QString host;
    bt::Uint16 port;
    QNetworkAccessManager *networkAccessManager;
    QByteArray reply;
    bool success;
    QString error;
};
}

#endif
