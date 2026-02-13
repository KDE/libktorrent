/*
 *  SPDX-FileCopyrightText: 2026 Jack Hill <jackhill3103@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <limits>

#include <QTest>

#include <bcodec/bencoder.h>

using namespace Qt::Literals::StringLiterals;

class BEncoderTest : public QEventLoop
{
    Q_OBJECT

private Q_SLOTS:

    void testWriteBoolTrue()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.write(true);
        QCOMPARE(buffer, "i1e");
    }

    void testWriteBoolFalse()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.write(false);
        QCOMPARE(buffer, "i0e");
    }

    void testWriteUint()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.write(999u);
        QCOMPARE(buffer, "i999e");
    }

    void testWriteUint64()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.write(std::numeric_limits<bt::Uint64>::max());
        QCOMPARE(buffer, "i18446744073709551615e");
    }

    void testWriteUint0()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.write(0u);
        QCOMPARE(buffer, "i0e");
    }

    void testWriteString()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.write(QByteArrayLiteral("info_hash"));
        QCOMPARE(buffer, "9:info_hash");
    }

    void testWriteList()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.beginList();
        encoder.write(QByteArrayLiteral("spam"));
        encoder.write(QByteArrayLiteral("eggs"));
        encoder.write(5u);
        encoder.end();
        QCOMPARE(buffer, "l4:spam4:eggsi5ee");
    }

    void testWriteDict()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.beginDict();
        encoder.write(QByteArrayLiteral("cow"));
        encoder.write(QByteArrayLiteral("moo"));
        encoder.write(QByteArrayLiteral("spam"));
        encoder.write(QByteArrayLiteral("eggs"));
        encoder.write(QByteArrayLiteral("kde2026"));
        encoder.write(30u);
        encoder.end();
        QCOMPARE(buffer, "d3:cow3:moo4:spam4:eggs7:kde2026i30ee");
    }

    void testWriteListInDict()
    {
        QByteArray buffer;
        bt::BEncoder encoder(std::make_unique<bt::BEncoderBufferOutput>(buffer));
        encoder.beginDict();
        encoder.write(QByteArrayLiteral("spam"));
        encoder.beginList();
        encoder.write(QByteArrayLiteral("a"));
        encoder.write(QByteArrayLiteral("b"));
        encoder.end();
        encoder.end();
        QCOMPARE(buffer, "d4:spaml1:a1:bee");
    }
};

QTEST_MAIN(BEncoderTest)

#include "bencodertest.moc"
