/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "addressresolver.h"

namespace net
{
AddressResolver::AddressResolver(const QString &host, bt::Uint16 port, QObject *parent, const char *slot)
    : QObject(parent)
    , lookup_id(-1)
    , succesfull(false)
{
    result.setPort(port);
    lookup_id = QHostInfo::lookupHost(host, this, SLOT(hostResolved(QHostInfo)));
    ongoing = true;
    connect(this, SIGNAL(resolved(net::AddressResolver *)), parent, slot);
}

AddressResolver::~AddressResolver()
{
    if (ongoing)
        QHostInfo::abortHostLookup(lookup_id);
}

void AddressResolver::hostResolved(const QHostInfo &res)
{
    ongoing = false;
    succesfull = res.error() == QHostInfo::NoError && res.addresses().count() > 0;
    if (!succesfull) {
        resolved(this);
    } else {
        result = net::Address(res.addresses().first(), result.port());
        resolved(this);
    }

    deleteLater();
}

void AddressResolver::resolve(const QString &host, Uint16 port, QObject *parent, const char *slot)
{
    new AddressResolver(host, port, parent, slot);
}

Address AddressResolver::resolve(const QString &host, Uint16 port)
{
    QHostInfo info = QHostInfo::fromName(host);
    if (info.error() == QHostInfo::NoError && info.addresses().size() > 0)
        return net::Address(info.addresses().first(), port);
    else
        return net::Address();
}

}
