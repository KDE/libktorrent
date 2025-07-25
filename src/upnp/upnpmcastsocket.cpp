/*
    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "upnpmcastsocket.h"
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QUrl>

#ifndef Q_OS_WIN
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <Ws2tcpip.h>
#endif

#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
static bool UrlCompare(const QUrl &a, const QUrl &b)
{
    if (a == b)
        return true;

    return a.scheme() == b.scheme() && a.host() == b.host() && a.password() == b.password() && a.port(80) == b.port(80) && a.path() == b.path()
        && a.query() == b.query(); // TODO check if ported correctly
}

class UPnPMCastSocket::UPnPMCastSocketPrivate
{
public:
    UPnPMCastSocketPrivate(bool verbose);
    ~UPnPMCastSocketPrivate();

    UPnPRouter *parseResponse(const QByteArray &arr);
    void joinUPnPMCastGroup(int fd);
    void leaveUPnPMCastGroup(int fd);
    void onXmlFileDownloaded(UPnPRouter *r, bool success);
    UPnPRouter *findDevice(const QUrl &location);

    QSet<UPnPRouter *> routers;
    QSet<UPnPRouter *> pending_routers; // routers which we are downloading the XML file from
    bool verbose;
};

UPnPMCastSocket::UPnPMCastSocket(bool verbose)
    : d(std::make_unique<UPnPMCastSocketPrivate>(verbose))
{
    QObject::connect(this, &UPnPMCastSocket::readyRead, this, &UPnPMCastSocket::onReadyRead);
    QObject::connect(this, &UPnPMCastSocket::errorOccurred, this, &UPnPMCastSocket::error);

    for (Uint32 i = 0; i < 10; i++) {
        if (!bind(1900 + i, QUdpSocket::ShareAddress))
            Out(SYS_PNP | LOG_IMPORTANT) << "Cannot bind to UDP port 1900 : " << errorString() << endl;
        else
            break;
    }

    d->joinUPnPMCastGroup(socketDescriptor());
}

UPnPMCastSocket::~UPnPMCastSocket()
{
    d->leaveUPnPMCastGroup(socketDescriptor());
}

void UPnPMCastSocket::discover()
{
    Out(SYS_PNP | LOG_NOTICE) << "Trying to find UPnP devices on the local network" << endl;

    // send a HTTP M-SEARCH message to 239.255.255.250:1900
    const char *upnp_data =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "ST:urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
        "MAN:\"ssdp:discover\"\r\n"
        "MX:3\r\n"
        "\r\n\0";

    const char *tr64_data =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "ST:urn:dslforum-org:device:InternetGatewayDevice:1\r\n"
        "MAN:\"ssdp:discover\"\r\n"
        "MX:3\r\n"
        "\r\n\0";

    if (d->verbose) {
        Out(SYS_PNP | LOG_NOTICE) << "Sending : " << endl;
        Out(SYS_PNP | LOG_NOTICE) << upnp_data << endl;

        Out(SYS_PNP | LOG_NOTICE) << "Sending : " << endl;
        Out(SYS_PNP | LOG_NOTICE) << tr64_data << endl;
    }

    writeDatagram(upnp_data, strlen(upnp_data), QHostAddress(u"239.255.255.250"_s), 1900);
    writeDatagram(tr64_data, strlen(tr64_data), QHostAddress(u"239.255.255.250"_s), 1900);
}

void UPnPMCastSocket::onXmlFileDownloaded(UPnPRouter *r, bool success)
{
    d->pending_routers.remove(r);
    if (!success) {
        // we couldn't download and parse the XML file so
        // get rid of it
        r->deleteLater();
    } else {
        // add it to the list and emit the signal
        QUrl location = r->getLocation();
        if (d->findDevice(location)) {
            r->deleteLater();
        } else {
            d->routers.insert(r);
            Q_EMIT discovered(r);
        }
    }
}

void UPnPMCastSocket::onReadyRead()
{
    if (pendingDatagramSize() == 0) {
        Out(SYS_PNP | LOG_NOTICE) << "0 byte UDP packet " << endl;
        // KDatagramSocket wrongly handles UDP packets with no payload
        // so we need to deal with it oursleves
        int fd = socketDescriptor();
        char tmp;
#ifndef Q_OS_WIN
        ::read(fd, &tmp, 1);
#else
        recv(fd, &tmp, 1, 0);
#endif
        return;
    }

    QByteArray data(pendingDatagramSize(), 0);
    if (readDatagram(data.data(), pendingDatagramSize()) == -1)
        return;

    if (d->verbose) {
        Out(SYS_PNP | LOG_NOTICE) << "Received : " << endl;
        Out(SYS_PNP | LOG_NOTICE) << QString::fromUtf8(data) << endl;
    }

    // try to make a router of it
    UPnPRouter *r = d->parseResponse(data);
    if (r) {
        QObject::connect(r, &UPnPRouter::xmlFileDownloaded, this, &UPnPMCastSocket::onXmlFileDownloaded);

        // download it's xml file
        r->downloadXMLFile();
        d->pending_routers.insert(r);
    }
}

void UPnPMCastSocket::error(QAbstractSocket::SocketError)
{
    Out(SYS_PNP | LOG_IMPORTANT) << "UPnPMCastSocket Error : " << errorString() << endl;
}

void UPnPMCastSocket::saveRouters(const QString &file)
{
    QFile fptr(file);
    if (!fptr.open(QIODevice::WriteOnly)) {
        Out(SYS_PNP | LOG_IMPORTANT) << "Cannot open file " << file << " : " << fptr.errorString() << endl;
        return;
    }

    // file format is simple : 2 lines per router,
    // one containing the server, the other the location
    QTextStream fout(&fptr);
    for (const UPnPRouter *r : std::as_const(d->routers)) {
        fout << r->getServer() << Qt::endl;
        fout << r->getLocation().toString() << Qt::endl;
    }
}

void UPnPMCastSocket::loadRouters(const QString &file)
{
    QFile fptr(file);
    if (!fptr.open(QIODevice::ReadOnly)) {
        Out(SYS_PNP | LOG_IMPORTANT) << "Cannot open file " << file << " : " << fptr.errorString() << endl;
        return;
    }

    // file format is simple : 2 lines per router,
    // one containing the server, the other the location
    QTextStream fin(&fptr);

    while (!fin.atEnd()) {
        QString server, location;
        server = fin.readLine();
        location = fin.readLine();

        UPnPRouter *r = new UPnPRouter(server, QUrl(location));
        // download it's xml file
        QObject::connect(r, &UPnPRouter::xmlFileDownloaded, this, &UPnPMCastSocket::onXmlFileDownloaded);
        r->downloadXMLFile();
        d->pending_routers.insert(r);
    }
}

Uint32 UPnPMCastSocket::getNumDevicesDiscovered() const
{
    return d->routers.count();
}

UPnPRouter *UPnPMCastSocket::findDevice(const QString &name)
{
    QUrl location(name);
    return d->findDevice(location);
}

void UPnPMCastSocket::setVerbose(bool v)
{
    d->verbose = v;
}

/////////////////////////////////////////////////////////////

UPnPMCastSocket::UPnPMCastSocketPrivate::UPnPMCastSocketPrivate(bool verbose)
    : verbose(verbose)
{
}

UPnPMCastSocket::UPnPMCastSocketPrivate::~UPnPMCastSocketPrivate()
{
    qDeleteAll(pending_routers);
    qDeleteAll(routers);
}

void UPnPMCastSocket::UPnPMCastSocketPrivate::joinUPnPMCastGroup(int fd)
{
    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(struct ip_mreq));

    inet_pton(AF_INET, "239.255.255.250", &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

#ifndef Q_OS_WIN
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) < 0)
#else
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(struct ip_mreq)) < 0)
#endif
    {
        Out(SYS_PNP | LOG_NOTICE) << "Failed to join multicast group 239.255.255.250" << endl;
    }
}

void UPnPMCastSocket::UPnPMCastSocketPrivate::leaveUPnPMCastGroup(int fd)
{
    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(struct ip_mreq));

    inet_pton(AF_INET, "239.255.255.250", &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

#ifndef Q_OS_WIN
    if (setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) < 0)
#else
    if (setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(struct ip_mreq)) < 0)
#endif
    {
        Out(SYS_PNP | LOG_NOTICE) << "Failed to leave multicast group 239.255.255.250" << endl;
    }
}

UPnPRouter *UPnPMCastSocket::UPnPMCastSocketPrivate::parseResponse(const QByteArray &arr)
{
    const QString response = QString::fromLatin1(arr);
    QList<QStringView> lines = QStringView(response).split(QStringLiteral("\r\n"));
    QString server;
    QUrl location;

    /*
    Out(SYS_PNP|LOG_DEBUG) << "Received : " << endl;
    for (Uint32 idx = 0;idx < lines.count(); idx++)
        Out(SYS_PNP|LOG_DEBUG) << lines[idx] << endl;
    */

    // first read first line and see if contains a HTTP 200 OK message
    auto line = lines.first();
    if (!line.contains(QLatin1String("HTTP"))) {
        // it is either a 200 OK or a NOTIFY
        if (!line.contains(QLatin1String("NOTIFY")) && !line.contains(QLatin1String("200")))
            return nullptr;
    } else if (line.contains(QLatin1String("M-SEARCH"))) // ignore M-SEARCH
        return nullptr;

    // quick check that the response being parsed is valid
    bool validDevice = false;
    for (int idx = 0; idx < lines.count() && !validDevice; idx++) {
        line = lines[idx];
        if ((line.contains(QLatin1String("ST:")) || line.contains(QLatin1String("NT:"))) && line.contains(QLatin1String("InternetGatewayDevice"))) {
            validDevice = true;
        }
    }
    if (!validDevice) {
        //  Out(SYS_PNP|LOG_IMPORTANT) << "Not a valid Internet Gateway Device" << endl;
        return nullptr;
    }

    // read all lines and try to find the server and location fields
    for (int i = 1; i < lines.count(); i++) {
        line = lines[i];
        if (line.startsWith(QLatin1String("location"), Qt::CaseInsensitive)) {
            location = QUrl(line.mid(line.indexOf(':'_L1) + 1).trimmed().toString()); // TODO fromLocalFile()?
            if (!location.isValid())
                return nullptr;
        } else if (line.startsWith(QLatin1String("server"), Qt::CaseInsensitive)) {
            server = line.mid(line.indexOf(':'_L1) + 1).trimmed().toString();
            if (server.length() == 0)
                return nullptr;
        }
    }

    if (findDevice(location)) {
        return nullptr;
    } else {
        Out(SYS_PNP | LOG_NOTICE) << "Detected IGD " << server << endl;
        // everything OK, make a new UPnPRouter
        return new UPnPRouter(server, location, verbose);
    }
}

UPnPRouter *UPnPMCastSocket::UPnPMCastSocketPrivate::findDevice(const QUrl &location)
{
    for (UPnPRouter *r : std::as_const(routers)) {
        if (UrlCompare(r->getLocation(), location))
            return r;
    }

    return nullptr;
}

}

#include "moc_upnpmcastsocket.cpp"
