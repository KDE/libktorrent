/*
 *  SPDX-FileCopyrightText: 2025 Jack Hill <jackhill3103@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <bcodec/bdecoder.h>
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

constexpr bool verbose = true;

class BDecoderTest : public QEventLoop
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"bdecodertest.log"_s, false, true, true);
    }

    void testEmpty()
    {
        const QByteArray buffer = "";
        bt::BDecoder decoder(buffer, verbose);
        QCOMPARE(decoder.decode(), nullptr);
    }

    void testBadInput_data()
    {
        QTest::addColumn<QByteArray>("buffer");

        QTest::addRow("String incomplete") << "5"_ba;
        QTest::addRow("String incomplete2") << "5:"_ba;
        QTest::addRow("String incomplete with value") << "5:a"_ba;

        QTest::addRow("Int incomplete") << "i"_ba;
        QTest::addRow("Int incomplete with value") << "i1"_ba;
        QTest::addRow("Int ends too soon") << "ie"_ba;

        QTest::addRow("List incomplete") << "l"_ba;
        QTest::addRow("List incomplete with value") << "l5i1e"_ba;
        QTest::addRow("List ends too soon") << "l5i1ee"_ba;

        QTest::addRow("Dict incomplete") << "d"_ba;
        QTest::addRow("Dict incomplete with key") << "d1:a"_ba;
        QTest::addRow("Dict incomplete with key-separator") << "d1:a:"_ba;
        QTest::addRow("Dict incomplete with key-separator-value") << "d1:a:i2e"_ba;
        QTest::addRow("Dict with int key") << "di1e:i2ee"_ba;
        QTest::addRow("Dict with list key") << "dl1i1e:i2eee"_ba;
        QTest::addRow("Dict with dict key") << "dd1:ai1eee"_ba;
    }

    void testBadInput()
    {
        QFETCH(QByteArray, buffer);
        bt::BDecoder decoder(buffer, verbose);
        bool error = false;
        try {
            QCOMPARE(decoder.decode(), nullptr);
        } catch (bt::Error &e) {
            bt::Out(SYS_GEN | LOG_NOTICE) << e.toString() << bt::endl;
            error = true;
        }
        QVERIFY(error);
    }

    void testList()
    {
        const QByteArrayView buffer = "li1e3:kdeli2eed1:ai3eee";
        bt::BDecoder dec(buffer.toByteArray(), verbose);

        bool error = false;
        try {
            const auto list = dec.decodeList();
            QVERIFY(list != nullptr);

            QCOMPARE(list->getInt(0), 1);
            QCOMPARE(list->getByteArray(1), "kde");
            const auto n2 = list->getList(2);
            QCOMPARE(buffer.mid(n2->getOffset(), n2->getLength()), "li2ee");
            const auto n3 = list->getDict(3);
            QCOMPARE(buffer.mid(n3->getOffset(), n3->getLength()), "d1:ai3ee");
        } catch (bt::Error &e) {
            bt::Out(SYS_GEN | LOG_NOTICE) << e.toString() << bt::endl;
            error = true;
        }
        QVERIFY(!error);
    }

    void testDict()
    {
        const QByteArrayView buffer = "d1:ai1e1:bli2e3:kdee1:cd2:aai11eee";
        bt::BDecoder dec(buffer.toByteArray(), verbose);

        bool error = false;
        try {
            const auto dict = dec.decodeDict();
            QVERIFY(dict != nullptr);

            QCOMPARE(dict->getInt("a"), 1);
            const auto node_b = dict->getList("b");
            QCOMPARE(buffer.mid(node_b->getOffset(), node_b->getLength()), "li2e3:kdee");
            const auto node_c = dict->getDict("c");
            QCOMPARE(buffer.mid(node_c->getOffset(), node_c->getLength()), "d2:aai11ee");
        } catch (bt::Error &e) {
            bt::Out(SYS_GEN | LOG_NOTICE) << e.toString() << bt::endl;
            error = true;
        }
        QVERIFY(!error);
    }

    void testNestedStructures()
    {
        const QByteArrayView buffer = "d1:ai1e1:bli2e3:kded2:aali5eeee1:cd3:aaai11eee";
        bt::BDecoder dec(buffer.toByteArray(), verbose);

        bool error = false;
        try {
            const auto dict = dec.decodeDict();
            QVERIFY(dict != nullptr);

            QCOMPARE(dict->getInt("a"), 1);

            bt::BNode *n = dict->getList("b");
            QCOMPARE(buffer.mid(n->getOffset(), n->getLength()), "li2e3:kded2:aali5eeee");
            QCOMPARE(dict->getList("b")->getInt(0), 2);
            QCOMPARE(dict->getList("b")->getByteArray(1), "kde");
            n = dict->getList("b")->getDict(2);
            QCOMPARE(buffer.mid(n->getOffset(), n->getLength()), "d2:aali5eee");
            n = dict->getList("b")->getDict(2)->getList("aa");
            QCOMPARE(buffer.mid(n->getOffset(), n->getLength()), "li5ee");
            QCOMPARE(dict->getList("b")->getDict(2)->getList("aa")->getInt(0), 5);

            n = dict->getDict("c");
            QCOMPARE(buffer.mid(n->getOffset(), n->getLength()), "d3:aaai11ee");
            QCOMPARE(dict->getDict("c")->getInt("aaa"), 11);
        } catch (bt::Error &e) {
            bt::Out(SYS_GEN | LOG_NOTICE) << e.toString() << bt::endl;
            error = true;
        }
        QVERIFY(!error);
    }
};

QTEST_MAIN(BDecoderTest)

#include "bdecodertest.moc"
