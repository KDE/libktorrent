/*
    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cstdlib>

#include <QDir>
#include <QDomElement>
#include <QNetworkRequest>
#include <QStringList>

#include <KIO/StoredTransferJob>
#include <KLocalizedString>

#include "httprequest.h"
#include "soap.h"
#include "upnpdescriptionparser.h"
#include "upnprouter.h"
#include <peer/accessmanager.h>
#include <torrent/globals.h>
#include <util/array.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/waitjob.h>
#include <version.h>

using namespace net;
using namespace Qt::Literals::StringLiterals;

namespace bt
{
struct Forwarding {
    net::Port port;
    HTTPRequest *pending_req;
    const UPnPService *service;
};

class UPnPRouter::UPnPRouterPrivate
{
public:
    UPnPRouterPrivate(const QString &server, const QUrl &location, bool verbose, UPnPRouter *parent);
    ~UPnPRouterPrivate();

    HTTPRequest *sendSoapQuery(const QString &query, const QString &soapact, const QString &controlurl, bool at_exit = false);
    void forward(const UPnPService *srv, const net::Port &port);
    void undoForward(const UPnPService *srv, const net::Port &port, bt::WaitJob *waitjob);
    void httpRequestDone(HTTPRequest *r, bool erase_fwd);
    void getExternalIP();

public:
    QString server;
    QUrl location;
    UPnPDeviceDescription desc;
    QList<UPnPService> services;
    QList<Forwarding> fwds;
    QList<HTTPRequest *> active_reqs;
    QString error;
    bool verbose;
    UPnPRouter *parent;
    QString external_ip;
};

////////////////////////////////////

UPnPService::UPnPService()
{
}

UPnPService::UPnPService(const UPnPService &s)
{
    this->servicetype = s.servicetype;
    this->controlurl = s.controlurl;
    this->eventsuburl = s.eventsuburl;
    this->serviceid = s.serviceid;
    this->scpdurl = s.scpdurl;
}

void UPnPService::setProperty(const QString &name, const QString &value)
{
    if (name == "serviceType"_L1) {
        servicetype = value;
    } else if (name == "controlURL"_L1) {
        controlurl = value;
    } else if (name == "eventSubURL"_L1) {
        eventsuburl = value;
    } else if (name == "SCPDURL"_L1) {
        scpdurl = value;
    } else if (name == "serviceId"_L1) {
        serviceid = value;
    }
}

void UPnPService::clear()
{
    servicetype = controlurl = eventsuburl = scpdurl = serviceid = QString();
}

UPnPService &UPnPService::operator=(const UPnPService &s)
{
    this->servicetype = s.servicetype;
    this->controlurl = s.controlurl;
    this->eventsuburl = s.eventsuburl;
    this->serviceid = s.serviceid;
    this->scpdurl = s.scpdurl;
    return *this;
}

///////////////////////////////////////

void UPnPDeviceDescription::setProperty(const QString &name, const QString &value)
{
    if (name == "friendlyName"_L1) {
        friendlyName = value;
    } else if (name == "manufacturer"_L1) {
        manufacturer = value;
    } else if (name == "modelDescription"_L1) {
        modelDescription = value;
    } else if (name == "modelName"_L1) {
        modelName = value;
    } else if (name == "modelNumber"_L1) {
        modelNumber = value;
    }
}

///////////////////////////////////////

UPnPRouter::UPnPRouter(const QString &server, const QUrl &location, bool verbose)
    : d(std::make_unique<UPnPRouterPrivate>(server, location, verbose, this))
{
}

UPnPRouter::~UPnPRouter()
{
}

void UPnPRouter::addService(UPnPService s)
{
    for (const UPnPService &os : std::as_const(d->services)) {
        if (s.servicetype == os.servicetype) {
            return;
        }
    }
    if (s.controlurl.startsWith('/'_L1)) {
        s.controlurl = "http://"_L1 + d->location.host() + ':'_L1 + QString::number(d->location.port()) + s.controlurl;
    }
    if (s.eventsuburl.startsWith('/'_L1)) {
        s.controlurl = "http://"_L1 + d->location.host() + ':'_L1 + QString::number(d->location.port()) + s.eventsuburl;
    }
    d->services.append(s);
}

void UPnPRouter::downloadFinished(const KJob *j)
{
    if (j->error()) {
        d->error = i18n("Failed to download %1: %2", d->location.toDisplayString(), j->errorString());
        Out(SYS_PNP | LOG_IMPORTANT) << d->error << endl;
        return;
    }

    const auto st = static_cast<const KIO::StoredTransferJob *>(j);
    // load in the file (target is always local)
    UPnPDescriptionParser desc_parse;
    bool ret = desc_parse.parse(st->data(), this);
    if (!ret) {
        d->error = i18n("Error parsing router description.");
    }

    Q_EMIT xmlFileDownloaded(this, ret);
    d->getExternalIP();
}

void UPnPRouter::downloadXMLFile()
{
    d->error = QString();
    // downlaod XML description into a temporary file in /tmp
    Out(SYS_PNP | LOG_DEBUG) << "Downloading XML file " << d->location << endl;
    KIO::Job *job = KIO::storedGet(d->location, KIO::NoReload, KIO::Overwrite | KIO::HideProgressInfo);
    connect(job, &KIO::Job::result, this, &UPnPRouter::downloadFinished);
}

void UPnPRouter::forward(const net::Port &port)
{
    if (!d->error.isEmpty()) {
        d->error = QString();
        Q_EMIT stateChanged();
    }

    bool found = false;
    Out(SYS_PNP | LOG_NOTICE) << "Forwarding port " << port.number << " (" << (port.proto == UDP ? "UDP" : "TCP") << ")" << endl;
    // first find the right service
    for (const UPnPService &s : std::as_const(d->services)) {
        if (s.servicetype.contains("WANIPConnection"_L1) || s.servicetype.contains("WANPPPConnection"_L1)) {
            d->forward(&s, port);
            found = true;
        }
    }

    if (!found) {
        d->error = i18n("Forwarding failed:\nDevice does not have a WANIPConnection or WANPPPConnection.");
        Out(SYS_PNP | LOG_IMPORTANT) << d->error << endl;
        Q_EMIT stateChanged();
    }
}

void UPnPRouter::undoForward(const net::Port &port, bt::WaitJob *waitjob)
{
    Out(SYS_PNP | LOG_NOTICE) << "Undoing forward of port " << port.number << " (" << (port.proto == UDP ? "UDP" : "TCP") << ")" << endl;

    QList<Forwarding>::iterator itr = d->fwds.begin();
    while (itr != d->fwds.end()) {
        Forwarding &wd = *itr;
        if (wd.port == port) {
            d->undoForward(wd.service, wd.port, waitjob);
            itr = d->fwds.erase(itr);
        } else {
            ++itr;
        }
    }

    Q_EMIT stateChanged();
}

void UPnPRouter::forwardResult(HTTPRequest *r)
{
    if (r->succeeded()) {
        d->httpRequestDone(r, false);
    } else {
        d->httpRequestDone(r, true);
        if (d->fwds.count() == 0) {
            d->error = r->errorString();
            Q_EMIT stateChanged();
        }
    }
}

void UPnPRouter::undoForwardResult(HTTPRequest *r)
{
    d->active_reqs.removeAll(r);
    r->deleteLater();
}

void UPnPRouter::getExternalIPResult(HTTPRequest *r)
{
    d->active_reqs.removeAll(r);
    if (r->succeeded()) {
        QDomDocument doc;
        if (!doc.setContent(r->replyData())) {
            Out(SYS_PNP | LOG_DEBUG) << "UPnP: GetExternalIP failed: invalid reply" << endl;
        } else {
            QDomNodeList nodes = doc.elementsByTagName(u"NewExternalIPAddress"_s);
            if (nodes.count() > 0) {
                d->external_ip = nodes.item(0).firstChild().nodeValue();
                Out(SYS_PNP | LOG_DEBUG) << "UPnP: External IP: " << d->external_ip << endl;
                // Keep track of external IP so AccessManager can block it, makes no sense to connect to ourselves
                AccessManager::instance().addExternalIP(d->external_ip);
            } else {
                Out(SYS_PNP | LOG_DEBUG) << "UPnP: GetExternalIP failed: no IP address returned" << endl;
            }
        }
    } else {
        Out(SYS_PNP | LOG_DEBUG) << "UPnP: GetExternalIP failed: " << r->errorString() << endl;
    }

    r->deleteLater();
}

QString UPnPRouter::getExternalIP() const
{
    return d->external_ip;
}

#if 0

void UPnPRouter::isPortForwarded(const net::Port& port)
{
    // first find the right service
    QList<UPnPService>::iterator i = findPortForwardingService();
    if (i == services.end())
        throw Error(i18n("Cannot find port forwarding service in the device's description."));

    // add all the arguments for the command
    QList<SOAP::Arg> args;
    SOAP::Arg a;
    a.element = "NewRemoteHost";
    args.append(a);

    // the external port
    a.element = "NewExternalPort";
    a.value = QString::number(port.number);
    args.append(a);

    // the protocol
    a.element = "NewProtocol";
    a.value = port.proto == TCP ? "TCP" : "UDP";
    args.append(a);

    UPnPService& s = *i;
    QString action = "GetSpecificPortMappingEntry";
    QString comm = SOAP::createCommand(action, s.servicetype, args);
    sendSoapQuery(comm, s.servicetype + "#" + action, s.controlurl);
}
#endif

void UPnPRouter::setVerbose(bool v)
{
    d->verbose = v;
}

QString UPnPRouter::getServer() const
{
    return d->server;
}

QUrl UPnPRouter::getLocation() const
{
    return d->location;
}

UPnPDeviceDescription &UPnPRouter::getDescription()
{
    return d->desc;
}

const UPnPDeviceDescription &UPnPRouter::getDescription() const
{
    return d->desc;
}

QString UPnPRouter::getError() const
{
    return d->error;
}

void UPnPRouter::visit(UPnPRouter::Visitor *visitor) const
{
    for (const Forwarding &fwd : std::as_const(d->fwds)) {
        visitor->forwarding(fwd.port, fwd.pending_req != nullptr, fwd.service);
    }
}

////////////////////////////////////

UPnPRouter::UPnPRouterPrivate::UPnPRouterPrivate(const QString &server, const QUrl &location, bool verbose, UPnPRouter *parent)
    : server(server)
    , location(location)
    , verbose(verbose)
    , parent(parent)
{
}

UPnPRouter::UPnPRouterPrivate::~UPnPRouterPrivate()
{
    for (HTTPRequest *r : std::as_const(active_reqs)) {
        r->deleteLater();
    }
}

HTTPRequest *UPnPRouter::UPnPRouterPrivate::sendSoapQuery(const QString &query, const QString &soapact, const QString &controlurl, bool at_exit)
{
    // if port is not set, 0 will be returned
    // thanks to Diego R. Brogna for spotting this bug
    if (location.port() <= 0) {
        location.setPort(80);
    }

    QUrl ctrlurl(controlurl);
    QString host = !ctrlurl.host().isEmpty() ? ctrlurl.host() : location.host();
    bt::Uint16 port = ctrlurl.port() != -1 ? ctrlurl.port() : location.port(80);

    QNetworkRequest networkReq;
    networkReq.setUrl(ctrlurl);
    networkReq.setRawHeader("Host", host.toLatin1() + QByteArrayLiteral(":") + QByteArray::number(port));
    networkReq.setRawHeader("User-Agent", bt::GetVersionString().toLatin1());
    networkReq.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("text/xml"));
    networkReq.setRawHeader("SOAPAction", QByteArrayLiteral("\"") + soapact.toLatin1() + QByteArrayLiteral("\""));

    HTTPRequest *r = new HTTPRequest(networkReq, query, host, port, verbose);
    if (!at_exit) {
        // Only listen for results when we are not exiting
        active_reqs.append(r);
    }
    r->start();
    return r;
}

void UPnPRouter::UPnPRouterPrivate::forward(const UPnPService *srv, const net::Port &port)
{
    // add all the arguments for the command
    QList<SOAP::Arg> args;
    SOAP::Arg a;
    a.element = u"NewRemoteHost"_s;
    args.append(a);

    // the external port
    a.element = u"NewExternalPort"_s;
    a.value = QString::number(port.number);
    args.append(a);

    // the protocol
    a.element = u"NewProtocol"_s;
    a.value = port.proto == net::TCP ? u"TCP"_s : u"UDP"_s;
    args.append(a);

    // the local port
    a.element = u"NewInternalPort"_s;
    a.value = QString::number(port.number);
    args.append(a);

    // the local IP address
    a.element = u"NewInternalClient"_s;
    a.value = u"$LOCAL_IP"_s; // will be replaced by our local ip in HTTPRequest
    args.append(a);

    a.element = u"NewEnabled"_s;
    a.value = u"1"_s;
    args.append(a);

    a.element = u"NewPortMappingDescription"_s;
    static Uint32 cnt = 0;
    a.value = u"KTorrent UPNP %1"_s.arg(cnt++); // TODO: change this
    args.append(a);

    a.element = u"NewLeaseDuration"_s;
    a.value = u"0"_s;
    args.append(a);

    QString action = u"AddPortMapping"_s;
    QString comm = SOAP::createCommand(action, srv->servicetype, args);

    Forwarding fw = {port, nullptr, srv};
    // erase old forwarding if one exists
    QList<Forwarding>::iterator itr = fwds.begin();
    while (itr != fwds.end()) {
        Forwarding &fwo = *itr;
        if (fwo.port == port && fwo.service == srv) {
            itr = fwds.erase(itr);
        } else {
            ++itr;
        }
    }

    fw.pending_req = sendSoapQuery(comm, srv->servicetype + '#'_L1 + action, srv->controlurl);
    connect(fw.pending_req, &HTTPRequest::result, parent, &UPnPRouter::forwardResult);
    fwds.append(fw);
}

void UPnPRouter::UPnPRouterPrivate::undoForward(const UPnPService *srv, const net::Port &port, bt::WaitJob *waitjob)
{
    // add all the arguments for the command
    QList<SOAP::Arg> args;
    SOAP::Arg a;
    a.element = u"NewRemoteHost"_s;
    args.append(a);

    // the external port
    a.element = u"NewExternalPort"_s;
    a.value = QString::number(port.number);
    args.append(a);

    // the protocol
    a.element = u"NewProtocol"_s;
    a.value = port.proto == net::TCP ? u"TCP"_s : u"UDP"_s;
    args.append(a);

    QString action = u"DeletePortMapping"_s;
    QString comm = SOAP::createCommand(action, srv->servicetype, args);
    HTTPRequest *r = sendSoapQuery(comm, srv->servicetype + '#'_L1 + action, srv->controlurl, waitjob != nullptr);

    if (waitjob) {
        waitjob->addExitOperation(r);
    } else {
        connect(r, &HTTPRequest::result, parent, &UPnPRouter::undoForwardResult);
    }
}

void UPnPRouter::UPnPRouterPrivate::getExternalIP()
{
    for (const UPnPService &s : std::as_const(services)) {
        if (s.servicetype.contains("WANIPConnection"_L1) || s.servicetype.contains("WANPPPConnection"_L1)) {
            QString action = u"GetExternalIPAddress"_s;
            QString comm = SOAP::createCommand(action, s.servicetype);
            HTTPRequest *r = sendSoapQuery(comm, s.servicetype + '#'_L1 + action, s.controlurl);
            connect(r, &HTTPRequest::result, parent, &UPnPRouter::getExternalIPResult);
            break;
        }
    }
}

void UPnPRouter::UPnPRouterPrivate::httpRequestDone(HTTPRequest *r, bool erase_fwd)
{
    int idx = 0;
    bool found = false;
    for (const Forwarding &fw : std::as_const(fwds)) {
        if (fw.pending_req == r) {
            found = true;
            break;
        }
        idx++;
    }

    if (found) {
        Forwarding &fw = fwds[idx];
        fw.pending_req = nullptr;
        if (erase_fwd) {
            fwds.removeAt(idx);
        }
    }

    active_reqs.removeAll(r);
    r->deleteLater();
}
}

#include "moc_upnprouter.cpp"
