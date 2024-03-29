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
    if (size <= 2 || packet[1] != 1)
        return;

    QByteArray tmp = QByteArray::fromRawData((const char *)packet, size);
    BNode *node = nullptr;
    try {
        BDecoder dec(tmp, false, 2);
        node = dec.decode();
        if (node && node->getType() == BNode::DICT) {
            BDictNode *dict = (BDictNode *)node;

            // ut_pex packet, emit signal to notify PeerManager
            BValueNode *val = dict->getValue("added");
            if (val) {
                QByteArray data = val->data().toByteArray();
                peer->emitPex(data);
            }
        }
    } catch (...) {
        // just ignore invalid packets
        Out(SYS_CON | LOG_DEBUG) << "Invalid ut_pex packet" << endl;
    }
    delete node;
}

bool UTPex::needsUpdate() const
{
    return bt::CurrentTime() - last_updated >= 60 * 1000;
}

void UTPex::visit(const bt::Peer::Ptr p)
{
    if (p.data() != peer) {
        npeers.insert(std::make_pair(p->getID(), p->getAddress()));
        if (peers.count(p->getID()) == 0) {
            // new one, add to added
            added.insert(std::make_pair(p->getID(), p->getAddress()));

            if (p->getAddress().ipVersion() == 4) {
                Uint8 flag = 0;
                if (p->isSeeder())
                    flag |= 0x02;
                if (p->getStats().fast_extensions)
                    flag |= 0x01;
                flags.insert(std::make_pair(p->getID(), flag));
            }
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

    if (!(peers.size() == 0 && added.size() == 0)) {
        // encode the whole lot
        QByteArray data;
        BEncoder enc(new BEncoderBufferOutput(data));
        enc.beginDict();
        enc.write(QByteArrayLiteral("added"));
        encode(enc, added);
        enc.write(QByteArrayLiteral("added.f"));
        if (added.size() == 0) {
            enc.write(QByteArray());
        } else {
            encodeFlags(enc, flags);
        }
        enc.write(QByteArrayLiteral("dropped"));
        encode(enc, peers);
        enc.end();

        peer->sendExtProtMsg(id, data);
    }

    peers = npeers;
    added.clear();
    flags.clear();
    npeers.clear();
}

void UTPex::encode(BEncoder &enc, const std::map<Uint32, net::Address> &ps)
{
    if (ps.size() == 0) {
        enc.write(QByteArray());
        return;
    }

    Uint8 *buf = new Uint8[ps.size() * 6];
    Uint32 size = 0;

    std::map<Uint32, net::Address>::const_iterator i = ps.begin();
    while (i != ps.end()) {
        const net::Address &addr = i->second;
        if (addr.ipVersion() == 4) {
            quint32 ip = htonl(addr.toIPv4Address());
            memcpy(buf + size, &ip, 4);
            WriteUint16(buf, size + 4, addr.port());
            size += 6;
        }
        ++i;
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
