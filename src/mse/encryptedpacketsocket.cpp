/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "encryptedpacketsocket.h"
#include <QtGlobal>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <util/log.h>
#include <util/sha1hash.h>
#ifndef Q_WS_WIN
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#else
#define IPTOS_THROUGHPUT 0x08
#endif
#include "rc4encryptor.h"
#include <net/socketmonitor.h>
#include <netinet/tcp.h>
#include <util/functions.h>

using namespace bt;
using namespace net;

namespace mse
{
Uint8 EncryptedPacketSocket::tos = IPTOS_THROUGHPUT;

EncryptedPacketSocket::EncryptedPacketSocket(int ip_version)
    : net::PacketSocket(true, ip_version)
    , enc(nullptr)
    , monitored(false)
{
    sock->setBlocking(false);
    sock->setTOS(tos);
    reinserted_data = nullptr;
    reinserted_data_size = 0;
    reinserted_data_read = 0;
}

EncryptedPacketSocket::EncryptedPacketSocket(int fd, int ip_version)
    : net::PacketSocket(fd, ip_version)
    , enc(nullptr)
    , monitored(false)
{
    sock->setBlocking(false);
    sock->setTOS(tos);
    reinserted_data = nullptr;
    reinserted_data_size = 0;
    reinserted_data_read = 0;
}

EncryptedPacketSocket::EncryptedPacketSocket(net::SocketDevice *sd)
    : net::PacketSocket(sd)
    , enc(nullptr)
    , monitored(false)
{
    sd->setBlocking(false);
    sd->setTOS(tos);
    reinserted_data = nullptr;
    reinserted_data_size = 0;
    reinserted_data_read = 0;
}

EncryptedPacketSocket::~EncryptedPacketSocket()
{
    if (monitored)
        stopMonitoring();

    delete[] reinserted_data;
    delete enc;
}

void EncryptedPacketSocket::startMonitoring(net::SocketReader *rdr)
{
    setReader(rdr);
    SocketMonitor::instance().add(this);
    monitored = true;
    if (reinserted_data) {
        if (enc)
            enc->decrypt(reinserted_data + reinserted_data_read, reinserted_data_size - reinserted_data_read);

        rdr->onDataReady(reinserted_data + reinserted_data_read, reinserted_data_size - reinserted_data_read);
        delete[] reinserted_data;
        reinserted_data = nullptr;
        reinserted_data_size = 0;
    }
}

void EncryptedPacketSocket::stopMonitoring()
{
    SocketMonitor::instance().remove(this);
    monitored = false;
    rdr = nullptr;
}

Uint32 EncryptedPacketSocket::sendData(const Uint8 *data, Uint32 len)
{
    if (enc) {
        // we need to make sure all data is sent because of the encryption
        Uint32 ds = 0;
        const Uint8 *ed = enc->encrypt(data, len);
        while (sock->ok() && ds < len) {
            Uint32 ret = sock->send(ed + ds, len - ds);
            ds += ret;
            if (ret == 0) {
                Out(SYS_CON | LOG_DEBUG) << "ret = 0" << endl;
            }
        }
        if (ds != len)
            Out(SYS_CON | LOG_DEBUG) << "ds != len" << endl;
        return ds;
    } else {
        Uint32 ret = sock->send(data, len);
        if (ret != len)
            Out(SYS_CON | LOG_DEBUG) << "ret != len" << endl;
        return ret;
    }
}

Uint32 EncryptedPacketSocket::readData(Uint8 *buf, Uint32 len)
{
    Uint32 ret2 = 0;
    if (reinserted_data) {
        Uint32 tr = reinserted_data_size - reinserted_data_read;
        if (tr < len) {
            memcpy(buf, reinserted_data + reinserted_data_read, tr);
            delete[] reinserted_data;
            reinserted_data = nullptr;
            reinserted_data_size = reinserted_data_read = 0;
            ret2 = tr;
            if (enc)
                enc->decrypt(buf, tr);
        } else {
            tr = len;
            memcpy(buf, reinserted_data + reinserted_data_read, tr);
            reinserted_data_read += tr;
            if (enc)
                enc->decrypt(buf, tr);
            return tr;
        }
    }

    if (len == ret2)
        return ret2;

    Uint32 ret = sock->recv(buf + ret2, len - ret2);
    if (ret + ret2 > 0 && enc)
        enc->decrypt(buf, ret + ret2);

    return ret;
}

Uint32 EncryptedPacketSocket::bytesAvailable() const
{
    Uint32 ba = sock->bytesAvailable();
    if (reinserted_data_size - reinserted_data_read > 0)
        return ba + (reinserted_data_size - reinserted_data_read);
    else
        return ba;
}

void EncryptedPacketSocket::close()
{
    sock->close();
}

bool EncryptedPacketSocket::connectTo(const QString &ip, Uint16 port)
{
    // do a safety check
    if (ip.isNull() || ip.length() == 0)
        return false;

    return connectTo(net::Address(ip, port));
}

bool EncryptedPacketSocket::connectTo(const net::Address &addr)
{
    // we don't wanna block the current thread so set non blocking
    sock->setBlocking(false);
    sock->setTOS(tos);
    if (sock->connectTo(addr))
        return true;

    return false;
}

void EncryptedPacketSocket::initCrypt(const bt::SHA1Hash &dkey, const bt::SHA1Hash &ekey)
{
    delete enc;
    enc = new RC4Encryptor(dkey, ekey);
}

void EncryptedPacketSocket::disableCrypt()
{
    delete enc;
    enc = nullptr;
}

bool EncryptedPacketSocket::ok() const
{
    return sock->ok();
}

QString EncryptedPacketSocket::getRemoteIPAddress() const
{
    return sock->getPeerName().toString();
}

bt::Uint16 EncryptedPacketSocket::getRemotePort() const
{
    return sock->getPeerName().port();
}

net::Address EncryptedPacketSocket::getRemoteAddress() const
{
    return sock->getPeerName();
}

void EncryptedPacketSocket::setRC4Encryptor(RC4Encryptor *e)
{
    delete enc;
    enc = e;
}

void EncryptedPacketSocket::reinsert(const Uint8 *d, Uint32 size)
{
    //      Out() << "Reinsert : " << size << endl;
    Uint32 off = 0;
    if (reinserted_data) {
        off = reinserted_data_size;
        reinserted_data = (Uint8 *)realloc(reinserted_data, reinserted_data_size + size);
        reinserted_data_size += size;
    } else {
        reinserted_data = new Uint8[size];
        reinserted_data_size = size;
    }
    memcpy(reinserted_data + off, d, size);
}

bool EncryptedPacketSocket::connecting() const
{
    return sock->state() == net::SocketDevice::CONNECTING;
}

bool EncryptedPacketSocket::connectSuccesFull() const
{
    return sock->connectSuccesFull();
}

void EncryptedPacketSocket::setRemoteAddress(const net::Address &addr)
{
    sock->setRemoteAddress(addr);
}

void EncryptedPacketSocket::preProcess(Packet::Ptr packet)
{
    if (enc)
        enc->encryptReplace(packet->getData(), packet->getDataLength());
}

void EncryptedPacketSocket::postProcess(Uint8 *data, Uint32 size)
{
    if (enc)
        enc->decrypt(data, size);
}

}
