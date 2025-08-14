/*
    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTHTTPREQUEST_H
#define KTHTTPREQUEST_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include <interfaces/exitoperation.h>
#include <util/constants.h>

namespace bt
{
/*!
 * \author Joris Guisson
 *
 * Simple HTTP request class.
 * TODO: switch to KIO for this
 */
class HTTPRequest : public bt::ExitOperation
{
    Q_OBJECT
public:
    /*!
     * Constructor, set the url and the request header.
     * \param hdr The http request header
     * \param payload The payload
     * \param host The host
     * \param port THe port
     * \param verbose Print traffic to the log
     */
    HTTPRequest(const QNetworkRequest &hdr, const QString &payload, const QString &host, bt::Uint16 port, bool verbose);
    /*!
     * Open a connection and send the request.
     */
    void start();

    /*!
        Get the reply data
    */
    QByteArray replyData() const
    {
        return reply;
    }

    /*!
        Did the request succeed
    */
    bool succeeded() const
    {
        return success;
    }

    /*!
        In case of failure this function will return an error string
    */
    QString errorString() const
    {
        return error;
    }

Q_SIGNALS:
    /*!
     * An OK reply was sent.
     * \param r The sender of the request
     */
    void result(bt::HTTPRequest *r);

public:
    void replyFinished();

private:
    void parseReply(int eoh);

private:
    QNetworkRequest hdr;
    QNetworkReply *networkReply;
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
