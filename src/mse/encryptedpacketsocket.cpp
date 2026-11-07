/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "encryptedpacketsocket.h"
#include <QtGlobal>
#include <cerrno>
#include <cstdlib>
#include <sys/types.h>
#include <util/log.h>
#include <util/sha1hash.h>
#ifndef Q_OS_WIN
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#else
#define IPTOS_THROUGHPUT 0x08
#endif
#include "rc4encryptor.h"
#include <net/socketmonitor.h>
#include <util/functions.h>

using namespace bt;
using namespace net;

namespace mse
{
Uint8 EncryptedPacketSocket::tos = IPTOS_THROUGHPUT;

EncryptedPacketSocket::EncryptedPacketSocket(int ip_version)
    : net::PacketSocket(true, ip_version)
    , monitored(false)
{
    sock->setBlocking(false);
    sock->setTOS(tos);
}

EncryptedPacketSocket::EncryptedPacketSocket(int fd, int ip_version)
    : net::PacketSocket(fd, ip_version)
    , monitored(false)
{
    sock->setBlocking(false);
    sock->setTOS(tos);
}

EncryptedPacketSocket::EncryptedPacketSocket(std::unique_ptr<net::SocketDevice> sd)
    : net::PacketSocket(std::move(sd))
    , monitored(false)
{
    sock->setBlocking(false);
    sock->setTOS(tos);
}

EncryptedPacketSocket::~EncryptedPacketSocket()
{
    if (monitored) {
        stopMonitoring();
    }
}

void EncryptedPacketSocket::startMonitoring(net::SocketReader *rdr)
{
    setReader(rdr);
    SocketMonitor::instance().add(this);
    monitored = true;
    if (!m_reinserted_data.isEmpty()) {
        const auto read_data = as_writable_bytes(QSpan{m_reinserted_data}).subspan(m_reinserted_data_read);
        if (enc) {
            enc->decrypt(read_data);
        }

        rdr->onDataReady(read_data);
        m_reinserted_data.clear();
    }
}

void EncryptedPacketSocket::stopMonitoring()
{
    SocketMonitor::instance().remove(this);
    monitored = false;
    rdr = nullptr;
}

Uint32 EncryptedPacketSocket::sendData(QByteArrayView data)
{
    if (enc) {
        // we need to make sure all data is sent because of the encryption
        Uint32 ds = 0;
        const auto ed = enc->encrypt(data);
        while (sock->ok() && ds < data.size()) {
            const Uint32 ret = sock->send(ed.sliced(ds));
            ds += ret;
            if (ret == 0) {
                Out(SYS_CON | LOG_DEBUG) << "ret = 0" << endl;
            }
        }
        if (ds != data.size()) {
            Out(SYS_CON | LOG_DEBUG) << "ds != data.size()" << endl;
        }
        return ds;
    } else {
        const Uint32 ret = sock->send(data);
        if (ret != data.size()) {
            Out(SYS_CON | LOG_DEBUG) << "ret != data.size()" << endl;
        }
        return ret;
    }
}

Uint32 EncryptedPacketSocket::readData(QSpan<std::byte> buf)
{
    Uint32 ret2 = 0;
    if (!m_reinserted_data.isEmpty()) {
        Uint32 tr = m_reinserted_data.size() - m_reinserted_data_read;
        if (tr < buf.size()) {
            const auto read_buf = QByteArrayView{m_reinserted_data}.sliced(m_reinserted_data_read, tr);
            memcpy(buf.data(), read_buf.data(), read_buf.size());
            m_reinserted_data.clear();
            ret2 = tr;
        } else {
            tr = buf.size();
            const auto read_buf = QByteArrayView{m_reinserted_data}.sliced(m_reinserted_data_read, tr);
            memcpy(buf.data(), read_buf.data(), read_buf.size());
            m_reinserted_data_read += tr;
            if (enc) {
                enc->decrypt(buf.first(tr));
            }
            return tr;
        }
    }

    if (buf.size() == ret2) {
        return ret2;
    }

    int ret = sock->recv(buf.subspan(ret2));
    if (ret < 0) {
        ret = 0;
    }

    if (ret + ret2 > 0 && enc) {
        enc->decrypt(buf.first(ret + ret2));
    }

    return ret + ret2;
}

Uint32 EncryptedPacketSocket::bytesAvailable() const
{
    const Uint32 ba = sock->bytesAvailable();
    if (m_reinserted_data.size() > m_reinserted_data_read) {
        return ba + (m_reinserted_data.size() - m_reinserted_data_read);
    } else {
        return ba;
    }
}

void EncryptedPacketSocket::close()
{
    sock->close();
}

bool EncryptedPacketSocket::connectTo(const QString &ip, Uint16 port)
{
    // do a safety check
    if (ip.isNull() || ip.length() == 0) {
        return false;
    }

    return connectTo(net::Address(ip, port));
}

bool EncryptedPacketSocket::connectTo(const net::Address &addr)
{
    // we don't wanna block the current thread so set non blocking
    sock->setBlocking(false);
    sock->setTOS(tos);

    return sock->connectTo(addr);
}

void EncryptedPacketSocket::initCrypt(const bt::SHA1Hash &dkey, const bt::SHA1Hash &ekey)
{
    enc = std::make_unique<RC4Encryptor>(dkey, ekey);
}

void EncryptedPacketSocket::disableCrypt()
{
    enc.reset();
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

void EncryptedPacketSocket::setRC4Encryptor(std::unique_ptr<RC4Encryptor> e)
{
    enc = std::move(e);
}

void EncryptedPacketSocket::setReinstertedData(QByteArrayView buf)
{
    Q_ASSERT(m_reinserted_data.isEmpty());
    Q_ASSERT(m_reinserted_data_read == 0);
    Q_ASSERT(!buf.isEmpty());
    //      Out() << "Reinsert : " << size << endl;
    m_reinserted_data = buf.toByteArray();
    m_reinserted_data_read = 0;
}

bool EncryptedPacketSocket::connecting() const
{
    return sock->state() == net::SocketDevice::State::CONNECTING;
}

bool EncryptedPacketSocket::connectSuccessful() const
{
    return sock->connectSuccessful();
}

void EncryptedPacketSocket::setRemoteAddress(const net::Address &addr)
{
    sock->setRemoteAddress(addr);
}

void EncryptedPacketSocket::preProcess(QSpan<std::byte> data)
{
    if (enc) {
        enc->encryptReplace(data);
    }
}

void EncryptedPacketSocket::postProcess(QSpan<std::byte> data)
{
    if (enc) {
        enc->decrypt(data);
    }
}

}
