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
        QByteArray buffer = "";
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
};

QTEST_MAIN(BDecoderTest)

#include "bdecodertest.moc"
