/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <array>

#include <QByteArrayView>
#include <QTest>

#include <bcodec/bdecoder.h>
#include <bcodec/bnode.h>
#include <dht/errmsg.h>
#include <dht/rpcmsg.h>
#include <dht/rpcmsgfactory.h>
#include <util/error.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

class RPCMsgTest : public QObject, public dht::RPCMethodResolver
{
    Q_OBJECT
private:
    dht::Method findMethod(const QByteArray &mtid) override
    {
        Q_UNUSED(mtid);
        return current_method;
    }

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"rpcmsgtest.log"_s, false, true);
    }

    void cleanupTestCase()
    {
    }

    void testErrMsg()
    {
        constexpr QByteArrayView msg = "d1:eli201e23:A Generic Error Ocurrede1:t2:aa1:y1:ee";

        try {
            bt::BDecoder dec(QByteArray(msg), false);

            const std::unique_ptr<bt::BDictNode> dict = dec.decodeDict();
            std::unique_ptr<dht::RPCMsg> msg = factory.build(dict.get(), nullptr);

            QVERIFY(msg->getType() == dht::ERR_MSG);
            const auto err = dynamic_cast<const dht::ErrMsg *>(msg.get());
            QVERIFY(err);
            QCOMPARE(err->message(), "A Generic Error Ocurred"_L1);
            QVERIFY(err->getMTID() == QByteArray("aa"));

        } catch (bt::Error &e) {
            QFAIL(e.toString().toLocal8Bit().data());
        }
    }

    void testWrongErrMsg()
    {
        constexpr std::array<QByteArrayView, 2> msgs = {
            "d1:t2:aa1:y1:ee",
            "d1:eli201e1:t2:aa1:y1:eee",
        };

        for (const auto msg : msgs) {
            bt::BDecoder dec(QByteArray(msg), false);
            const std::unique_ptr<bt::BDictNode> dict = dec.decodeDict();
            try {
                const std::unique_ptr<dht::RPCMsg> msg = factory.build(dict.get(), this);
                QFAIL("No exception thrown");
            } catch (bt::Error &) {
                // OK
            }
        }
    }

    void testPing()
    {
        constexpr std::array<QByteArrayView, 2> msgs = {
            "d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe",
            "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re",
        };
        current_method = dht::PING;

        for (const auto msg : msgs) {
            bt::BDecoder dec(QByteArray(msg), false);
            const std::unique_ptr<bt::BDictNode> dict = dec.decodeDict();
            try {
                std::unique_ptr<dht::RPCMsg> msg = factory.build(dict.get(), this);
                QVERIFY(msg);
                QVERIFY(msg->getMTID() == QByteArray("aa"));
                QVERIFY(msg->getMethod() == dht::PING);
            } catch (bt::Error &e) {
                QFAIL(e.toString().toLocal8Bit().data());
            }
        }
    }

    void testFindNode()
    {
        constexpr std::array<QByteArrayView, 2> msgs = {
            "d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe",
            "d1:rd2:id20:0123456789abcdefghij5:nodes9:def456...e1:t2:aa1:y1:re",
        };

        current_method = dht::FIND_NODE;

        for (const auto msg : msgs) {
            bt::BDecoder dec(QByteArray(msg), false);
            const std::unique_ptr<bt::BDictNode> dict = dec.decodeDict();
            try {
                std::unique_ptr<dht::RPCMsg> msg = factory.build(dict.get(), this);
                QVERIFY(msg);
                QVERIFY(msg->getMTID() == QByteArray("aa"));
                QVERIFY(msg->getMethod() == dht::FIND_NODE);
            } catch (bt::Error &e) {
                QFAIL(e.toString().toLocal8Bit().data());
            }
        }
    }

    void testGetPeers()
    {
        constexpr std::array<QByteArrayView, 3> msgs = {
            "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe",
            "d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:axje.u6:idhtnmee1:t2:aa1:y1:re",
            "d1:rd2:id20:abcdefghij01234567895:nodes9:def456...5:token8:aoeusnthe1:t2:aa1:y1:re",
        };
        current_method = dht::GET_PEERS;

        for (const auto msg : msgs) {
            bt::BDecoder dec(QByteArray(msg), false);
            const std::unique_ptr<bt::BDictNode> dict = dec.decodeDict();
            try {
                std::unique_ptr<dht::RPCMsg> msg = factory.build(dict.get(), this);
                QVERIFY(msg);
                QVERIFY(msg->getMTID() == QByteArray("aa"));
                QVERIFY(msg->getMethod() == dht::GET_PEERS);
            } catch (bt::Error &e) {
                QFAIL(e.toString().toLocal8Bit().data());
            }
        }
    }

    void testAnnounce()
    {
        constexpr std::array<QByteArrayView, 2> msgs = {
            "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
            "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re",
        };
        current_method = dht::ANNOUNCE_PEER;

        for (const auto msg : msgs) {
            bt::BDecoder dec(QByteArray(msg), false);
            const std::unique_ptr<bt::BDictNode> dict = dec.decodeDict();
            try {
                std::unique_ptr<dht::RPCMsg> msg = factory.build(dict.get(), this);
                QVERIFY(msg);
                QVERIFY(msg->getMTID() == QByteArray("aa"));
                QVERIFY(msg->getMethod() == dht::ANNOUNCE_PEER);
            } catch (bt::Error &e) {
                QFAIL(e.toString().toLocal8Bit().data());
            }
        }
    }

private:
    dht::RPCMsgFactory factory;
    dht::Method current_method;
};

QTEST_MAIN(RPCMsgTest)

#include "rpcmsgtest.moc"
