/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTHTTPCONNECTION_H
#define BTHTTPCONNECTION_H

#include <QRecursiveMutex>
#include <QTimer>
#include <QUrl>
#include <net/addressresolver.h>
#include <net/streamsocket.h>

class QUrl;

namespace bt
{
/*!
    \author Joris Guisson

    HTTP connection for webseeding. We do not use KIO here, because we want to be able to apply
    the maximum upload and download rate to webseeds;
*/
class HttpConnection : public QObject, public net::SocketReader, public net::StreamSocketListener
{
    Q_OBJECT
public:
    HttpConnection();
    ~HttpConnection() override;

    //! Get the last http response code
    int responseCode() const
    {
        return response_code;
    }

    //! Is this connection redirected
    bool isRedirected() const
    {
        return redirected;
    }

    //! Get the redirected url
    QUrl redirectedUrl() const
    {
        return redirected_url;
    }

    /*!
     * Set the group ID's of the socket
     * \param up Upload group id
     * \param down Download group id
     */
    void setGroupIDs(Uint32 up, Uint32 down);

    /*!
     * Connect to a webseed
     * \param url Url of the webseeder
     */
    void connectTo(const QUrl &url);

    /*!
     * Connect to a proxy.
     * \param proxy The HTTP proxy to use (null means don't use any)
     * \param proxy_port The port of the HTTP proxy
     */
    void connectToProxy(const QString &proxy, Uint16 proxy_port);

    //! Check if the connection is OK
    bool ok() const;

    //! See if we are connected
    bool connected() const;

    //! Has the connection been closed
    bool closed() const;

    //! Ready to do another request
    bool ready() const;

    /*!
     * Do a HTTP GET request
     * \param host The hostname of the webseed
     * \param path The path of the file
     * \param query The query string for the url
     * \param start Offset into file
     * \param len Length of data to download
     */
    bool get(const QString &host, const QString &path, const QString &query, bt::Uint64 start, bt::Uint64 len);

    void onDataReady(Uint8 *buf, Uint32 size) override;
    void connectFinished(bool succeeded) override;
    void dataSent() override;

    /*!
     * Get some part of the
     * \param data Bytearray to copy the data into
     * \return true if data was filled in
     */
    bool getData(QByteArray &data);

    //! Get the current download rate
    int getDownloadRate() const;

    //! Get the status string
    const QString getStatusString() const;

private:
    void connectTimeout();
    void replyTimeout();

Q_SIGNALS:
    void startReplyTimer(int timeout);
    void stopReplyTimer();
    void stopConnectTimer();

private Q_SLOTS:
    void hostResolved(net::AddressResolver *ar);

private:
    enum State {
        IDLE,
        RESOLVING,
        CONNECTING,
        ACTIVE,
        ERROR,
        CLOSED,
    };

    struct HttpGet {
        QString host;
        QString path;
        QString query;
        bt::Uint64 start;
        bt::Uint64 len;
        bt::Uint64 data_received;
        QByteArray buffer;
        QByteArray piece_data;
        bool response_header_received;
        bool request_sent;
        QString failure_reason;
        bool redirected;
        QUrl redirected_to;
        bt::Uint64 content_length;
        int response_code;

        HttpGet(const QString &host, const QString &path, const QString &query, bt::Uint64 start, bt::Uint64 len, bool using_proxy);
        virtual ~HttpGet();

        bool onDataReady(Uint8 *buf, Uint32 size);
        bool finished() const
        {
            return data_received >= len;
        }
    };

    net::StreamSocket *sock;
    State state;
    mutable QRecursiveMutex mutex;
    HttpGet *request;
    bool using_proxy;
    QString status;
    QTimer connect_timer;
    QTimer reply_timer;
    Uint32 up_gid, down_gid;
    bool close_when_finished;
    bool redirected;
    QUrl redirected_url;
    int response_code;
};
}

#endif
