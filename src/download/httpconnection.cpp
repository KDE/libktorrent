/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "httpconnection.h"
#include "httpresponseheader.h"
#include <KLocalizedString>
#include <QTimer>
#include <QUrl>
#include <QtAlgorithms>
#include <net/socketmonitor.h>
#include <util/functions.h>
#include <util/log.h>

#include "version.h"

using namespace Qt::Literals::StringLiterals;

namespace bt
{
HttpConnection::HttpConnection()
    : sock(nullptr)
    , state(IDLE)
    , mutex()
    , request(nullptr)
    , using_proxy(false)
    , response_code(0)
{
    status = i18n("Not connected");
    connect(&reply_timer, &QTimer::timeout, this, &HttpConnection::replyTimeout);
    connect(&connect_timer, &QTimer::timeout, this, &HttpConnection::connectTimeout);
    connect(this, &HttpConnection::startReplyTimer, &reply_timer, qOverload<int>(&QTimer::start), Qt::QueuedConnection);
    connect(this, &HttpConnection::stopReplyTimer, &reply_timer, &QTimer::stop, Qt::QueuedConnection);
    connect(this, &HttpConnection::stopConnectTimer, &connect_timer, &QTimer::stop, Qt::QueuedConnection);
    up_gid = down_gid = 0;
    close_when_finished = false;
    redirected = false;
}

HttpConnection::~HttpConnection()
{
    if (sock) {
        net::SocketMonitor::instance().remove(sock);
        delete sock;
    }

    delete request;
}

void HttpConnection::setGroupIDs(Uint32 up, Uint32 down)
{
    up_gid = up;
    down_gid = down;
    if (sock) {
        sock->setGroupID(up_gid, true);
        sock->setGroupID(down_gid, false);
    }
}

const QString HttpConnection::getStatusString() const
{
    QMutexLocker locker(&mutex);
    return status;
}

bool HttpConnection::ok() const
{
    QMutexLocker locker(&mutex);
    return state != ERROR;
}

bool HttpConnection::connected() const
{
    QMutexLocker locker(&mutex);
    return state == ACTIVE;
}

bool HttpConnection::closed() const
{
    QMutexLocker locker(&mutex);
    return state == CLOSED || (sock && !sock->socketDevice()->ok());
}

bool HttpConnection::ready() const
{
    QMutexLocker locker(&mutex);
    return !request;
}

void HttpConnection::connectToProxy(const QString &proxy, Uint16 proxy_port)
{
    if (OpenFileAllowed()) {
        using_proxy = true;
        net::AddressResolver::resolve(proxy, proxy_port, this, SLOT(hostResolved(net::AddressResolver *)));
        state = RESOLVING;
        status = i18n("Resolving proxy %1:%2", proxy, proxy_port);
    } else {
        Out(SYS_CON | LOG_IMPORTANT) << "HttpConnection: not enough system resources available" << endl;
        state = ERROR;
        status = i18n("Not enough system resources available");
    }
}

void HttpConnection::connectTo(const QUrl &url)
{
    if (OpenFileAllowed()) {
        using_proxy = false;
        net::AddressResolver::resolve(url.host(), url.port() <= 0 ? 80 : url.port(), this, SLOT(hostResolved(net::AddressResolver *)));
        state = RESOLVING;
        status = i18n("Resolving hostname %1", url.host());
    } else {
        Out(SYS_CON | LOG_IMPORTANT) << "HttpConnection: not enough system resources available" << endl;
        state = ERROR;
        status = i18n("Not enough system resources available");
    }
}

void HttpConnection::onDataReady(Uint8 *buf, Uint32 size)
{
    QMutexLocker locker(&mutex);

    if (state != ERROR && request) {
        if (size == 0) {
            // connection closed
            state = CLOSED;
            status = i18n("Connection closed");
        } else {
            if (!request->onDataReady(buf, size)) {
                state = ERROR;
                status = i18n("Error: request failed: %1", request->failure_reason);
                response_code = request->response_code;
            } else if (request->response_header_received)
                Q_EMIT stopReplyTimer();
        }
    }
}

void HttpConnection::dataSent()
{
    QMutexLocker locker(&mutex);
    if (state == ACTIVE && request) {
        request->buffer.clear();
        // wait 60 seconds for a reply
        Q_EMIT startReplyTimer(60 * 1000);
    }
}

void HttpConnection::connectFinished(bool succeeded)
{
    QMutexLocker locker(&mutex);
    if (state == CONNECTING) {
        if (succeeded) {
            state = ACTIVE;
            status = i18n("Connected");
            if (request && !request->request_sent) {
                sock->addData(request->buffer);
                request->request_sent = true;
            }
        } else {
            Out(SYS_CON | LOG_IMPORTANT) << "HttpConnection: failed to connect to webseed " << endl;
            state = ERROR;
            status = i18n("Error: Failed to connect to webseed");
        }
        Q_EMIT stopConnectTimer();
    }
}

void HttpConnection::hostResolved(net::AddressResolver *ar)
{
    if (ar->succeeded()) {
        net::Address addr = ar->address();
        if (!sock) {
            sock = new net::StreamSocket(true, addr.ipVersion(), this);
            sock->socketDevice()->setBlocking(false);
            sock->setReader(this);
            sock->setGroupID(up_gid, true);
            sock->setGroupID(down_gid, false);
        }

        if (sock->socketDevice()->connectTo(addr)) {
            status = i18n("Connected");
            state = ACTIVE;
            net::SocketMonitor::instance().add(sock);
            net::SocketMonitor::instance().signalPacketReady();
        } else if (sock->socketDevice()->state() == net::SocketDevice::CONNECTING) {
            status = i18n("Connecting");
            state = CONNECTING;
            net::SocketMonitor::instance().add(sock);
            net::SocketMonitor::instance().signalPacketReady();
            // 60 second connect timeout
            connect_timer.start(60000);
        } else {
            Out(SYS_CON | LOG_IMPORTANT) << "HttpConnection: failed to connect to webseed" << endl;
            state = ERROR;
            status = i18n("Failed to connect to webseed");
        }
    } else {
        Out(SYS_CON | LOG_IMPORTANT) << "HttpConnection: failed to resolve hostname of webseed" << endl;
        state = ERROR;
        status = i18n("Failed to resolve hostname of webseed");
    }
}

bool HttpConnection::get(const QString &host, const QString &path, const QString &query, bt::Uint64 start, bt::Uint64 len)
{
    QMutexLocker locker(&mutex);
    if (state == ERROR || request)
        return false;

    request = new HttpGet(host, path, query, start, len, using_proxy);
    if (sock) {
        sock->addData(request->buffer);
        request->request_sent = true;
    }
    return true;
}

bool HttpConnection::getData(QByteArray &data)
{
    QMutexLocker locker(&mutex);
    if (!request)
        return false;

    HttpGet *g = request;
    if (g->redirected) {
        // wait until we have the entire content if we are redirected
        if (g->data_received < g->content_length)
            return false;

        // we have the content so we can redirect the connection
        redirected_url = g->redirected_to;
        redirected = true;
        return false;
    }

    if (g->piece_data.size() == 0)
        return false;

    data = g->piece_data;
    g->piece_data.clear();

    // if all the data has been received and passed on to something else
    // remove the current request from the queue
    if (g->piece_data.size() == 0 && g->finished()) {
        delete g;
        request = nullptr;
        if (close_when_finished) {
            state = CLOSED;
            Out(SYS_CON | LOG_DEBUG) << "HttpConnection: closing connection due to redirection" << endl;
            // reset connection
            sock->socketDevice()->reset();
        }
    }

    return true;
}

int HttpConnection::getDownloadRate() const
{
    if (sock) {
        sock->updateSpeeds(bt::CurrentTime());
        return sock->getDownloadRate();
    } else
        return 0;
}

void HttpConnection::connectTimeout()
{
    QMutexLocker locker(&mutex);
    if (state == CONNECTING) {
        status = i18n("Error: failed to connect, server not responding");
        state = ERROR;
    }
    connect_timer.stop();
}

void HttpConnection::replyTimeout()
{
    QMutexLocker locker(&mutex);
    if (!request || !request->response_header_received) {
        status = i18n("Error: request timed out");
        state = ERROR;
        reply_timer.stop();
    }
}

////////////////////////////////////////////

HttpConnection::HttpGet::HttpGet(const QString &host, const QString &path, const QString &query, bt::Uint64 start, bt::Uint64 len, bool using_proxy)
    : host(host)
    , path(path)
    , query(query)
    , start(start)
    , len(len)
    , data_received(0)
    , response_header_received(false)
    , request_sent(false)
    , response_code(0)
{
    QUrl url;
    url.setPath(path);
    url.setQuery(query);

    buffer.clear();
    buffer += QByteArrayLiteral("GET ") + (using_proxy ? url.toEncoded() : (url.path(QUrl::FullyEncoded).toLatin1() + '?' + url.query(QUrl::FullyEncoded).toLatin1())) + " HTTP/1.1\r\n"
              "Host: " + host.toLatin1() + "\r\n"
              "Range: bytes=" + QByteArray::number(start) + '-' + QByteArray::number(start + len - 1) + "\r\n"
              "User-Agent: " + bt::GetVersionString().toLatin1() + "\r\n"
              "Accept:  text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\n"
              "Accept-Language: en-us,en;q=0.5\r\n"
              "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
              + (using_proxy ?
                 "Keep-Alive: 300\r\n"
                 "Proxy-Connection: keep-alive\r\n\r\n" :
                 "Connection: Keep-Alive\r\n\r\n");

    redirected = false;
    content_length = 0;
    Out(SYS_CON | LOG_DEBUG) << "HttpConnection: sending http request:" << endl;
    Out(SYS_CON | LOG_DEBUG) << buffer.constData() << endl;
}

HttpConnection::HttpGet::~HttpGet()
{
}

bool HttpConnection::HttpGet::onDataReady(Uint8 *buf, Uint32 size)
{
    if (!response_header_received) {
        // append the data
        buffer.append(QByteArray::fromRawData((char *)buf, size));
        // look for the end of the header
        int idx = buffer.indexOf("\r\n\r\n");
        if (idx == -1) // haven't got the full header yet
            return true;

        response_header_received = true;
        HttpResponseHeader hdr(QString::fromLatin1(buffer.mid(0, idx + 4)));

        if (hdr.hasKey(u"Content-Length"_s))
            content_length = hdr.value(u"Content-Length"_s).toInt();
        else
            content_length = 0;

        Out(SYS_CON | LOG_DEBUG) << "HttpConnection: http reply header received" << endl;
        Out(SYS_CON | LOG_DEBUG) << buffer.mid(0, idx + 4).constData() << endl;
        response_code = hdr.statusCode();
        if ((hdr.statusCode() >= 300 && hdr.statusCode() <= 303) || hdr.statusCode() == 307) {
            // we got redirected to somewhere else
            if (!hdr.hasKey(u"Location"_s)) {
                failure_reason = i18n("Redirected without a new location.");
                return false;
            } else {
                Out(SYS_CON | LOG_DEBUG) << "Redirected to " << hdr.value(u"Location"_s) << endl;
                redirected = true;
                redirected_to = QUrl(hdr.value(u"Location"_s));
            }
        } else if (!(hdr.statusCode() == 200 || hdr.statusCode() == 206)) {
            failure_reason = hdr.reasonPhrase();
            return false;
        }

        if (buffer.size() - (idx + 4) > 0) {
            // more data then the header has arrived so append it to piece_data
            data_received += buffer.size() - (idx + 4);
            piece_data.append(buffer.mid(idx + 4));
        }
    } else {
        // append the data to the list
        data_received += size;
        piece_data.append(QByteArray((char *)buf, size));
    }
    return true;
}
}

#include "moc_httpconnection.cpp"
