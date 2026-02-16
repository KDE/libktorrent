/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "utpex.h"

#include "peer.h"
#include "peermanager.h"
#include <bcodec/bdecoder.h>
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <net/address.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
bool UTPex::pex_enabled = true;

UTPex::UTPex(Peer *peer, Uint32 id)
    : PeerProtocolExtension(id, peer)
    , last_updated(0)
{
}

UTPex::~UTPex()
{
}

void UTPex::handlePacket(const Uint8 *packet, Uint32 size)
{
    if (size <= 2 || packet[1] != 1) {
        return;
    }

    QByteArray tmp = QByteArray::fromRawData((const char *)packet, size);
    try {
        BDecoder dec(tmp, false, 2);
        const std::unique_ptr<BDictNode> dict = dec.decodeDict();
        if (dict) {
            // ut_pex packet, emit signal to notify PeerManager
            BValueNode *peers4 = dict->getValue("added");
            if (peers4) {
                QByteArray data = peers4->data().toByteArray();
                if (!data.isEmpty()) {
                    peer->emitPex(data, 4);
                }
            }
            BValueNode *peers6 = dict->getValue("added6");
            if (peers6) {
                QByteArray data = peers6->data().toByteArray();
                if (!data.isEmpty()) {
                    peer->emitPex(data, 6);
                }
            }
        }
    } catch (...) {
        // just ignore invalid packets
        Out(SYS_CON | LOG_DEBUG) << "Invalid ut_pex packet" << endl;
    }
}

bool UTPex::needsUpdate() const
{
    return bt::CurrentTime() - last_updated >= 60 * 1000;
}

void UTPex::visit(const bt::Peer *p)
{
    const auto ip_version = p->getAddress().ipVersion();

    if (ip_version == 4) {
        visit(p, peers4, added4, flags4, npeers4);
    } else if (ip_version == 6) {
        visit(p, peers6, added6, flags6, npeers6);
    }
}

void UTPex::visit(const bt::Peer *p,
                  std::map<Uint32, net::Address> &peers,
                  std::map<Uint32, net::Address> &added,
                  std::map<Uint32, Uint8> &flags,
                  std::map<Uint32, net::Address> &npeers)
{
    if (p != peer) {
        npeers.insert(std::make_pair(p->getID(), p->getAddress()));
        if (peers.count(p->getID()) == 0) {
            // new one, add to added
            added.insert(std::make_pair(p->getID(), p->getAddress()));

            Uint8 flag = 0;
            if (p->isSeeder()) {
                flag |= 0x02;
            }
            if (p->getStats().fast_extensions) {
                flag |= 0x01;
            }
            flags.insert(std::make_pair(p->getID(), flag));
        } else {
            // erase from old list, so only the dropped ones are left
            peers.erase(p->getID());
        }
    }
}

void UTPex::update()
{
    PeerManager *pman = peer->getPeerManager();
    last_updated = bt::CurrentTime();

    pman->visit(*this);

    QByteArray data;
    BEncoder enc(std::make_unique<BEncoderBufferOutput>(data));
    enc.beginDict();

    encodePeers(enc, peers4, added4, flags4, 4);
    encodePeers(enc, peers6, added6, flags6, 6);

    enc.end();

    // No peers means bencode result is just "de"
    if (data.size() > 2) {
        peer->sendExtProtMsg(id, data);
    }

    peers4 = std::move(npeers4);
    added4.clear();
    flags4.clear();
    npeers4.clear();

    peers6 = std::move(npeers6);
    added6.clear();
    flags6.clear();
    npeers6.clear();
}

void UTPex::encodePeers(BEncoder &enc,
                        const std::map<Uint32, net::Address> &dropped,
                        const std::map<Uint32, net::Address> &added,
                        const std::map<Uint32, Uint8> &flags,
                        int ip_version)
{
    if (!added.empty()) {
        // encode the whole lot
        enc.write(ip_version == 4 ? QByteArrayLiteral("added") : QByteArrayLiteral("added6"));
        encode(enc, added, ip_version);

        enc.write(ip_version == 4 ? QByteArrayLiteral("added.f") : QByteArrayLiteral("added6.f"));
        encodeFlags(enc, flags);
    }

    if (!dropped.empty()) {
        enc.write(ip_version == 4 ? QByteArrayLiteral("dropped") : QByteArrayLiteral("dropped6"));
        encode(enc, dropped, ip_version);
    }
}

void UTPex::encode(BEncoder &enc, const std::map<Uint32, net::Address> &ps, int ip_version)
{
    if (ps.size() == 0) {
        enc.write(QByteArray());
        return;
    }

    Uint8 *buf = nullptr;
    if (ip_version == 4) {
        buf = new Uint8[ps.size() * 6];
    } else if (ip_version == 6) {
        buf = new Uint8[ps.size() * 18];
    }

    Uint32 size = 0;

    for (const auto &[id, addr] : ps) {
        if (addr.ipVersion() != ip_version) {
            continue;
        }
        if (ip_version == 4) {
            quint32 ip = htonl(addr.toIPv4Address());
            memcpy(buf + size, &ip, 4);
            WriteUint16(buf, size + 4, addr.port());
            size += 6;
        } else if (ip_version == 6) {
            const Q_IPV6ADDR ip6 = addr.toIPv6Address();
            const quint8 *ip = ip6.c;
            memcpy(buf + size, ip, 16);
            WriteUint16(buf, size + 16, addr.port());
            size += 18;
        }
    }

    enc.write(buf, size);
    delete[] buf;
}

void UTPex::encodeFlags(BEncoder &enc, const std::map<Uint32, Uint8> &flags)
{
    if (flags.size() == 0) {
        enc.write(QByteArray());
        return;
    }

    Uint8 *buf = new Uint8[flags.size()];
    Uint32 idx = 0;

    std::map<Uint32, Uint8>::const_iterator i = flags.begin();
    while (i != flags.end()) {
        buf[idx++] = i->second;
        ++i;
    }

    enc.write(buf, flags.size());
    delete[] buf;
}
}
