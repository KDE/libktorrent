/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QRandomGenerator>
#include <QTest>

#include <string.h>
#include <iostream>
#include <dht/key.h>
#include <mse/rc4encryptor.h>
#include <util/functions.h>
#include <util/error.h>

class RC4EncryptorTest : public QObject
{
    Q_OBJECT
public:
    bt::SHA1Hash randomKey()
    {
        bt::Uint32 hash[5];
        for (int i = 0; i < 5; i++) {
            hash[i] = QRandomGenerator::global()->generate();
        }
        return bt::SHA1Hash(reinterpret_cast<bt::Uint8 *>(hash));
    }

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(bt::InitLibKTorrent());
    }

    void cleanupTestCase()
    {
    }

    void testRC4()
    {
        try {
            bt::SHA1Hash dkey = randomKey();
            bt::SHA1Hash ekey = randomKey();
            mse::RC4Encryptor a(dkey, ekey);
            mse::RC4Encryptor b(ekey, dkey);

            bt::Uint8 tmp[1024];
            for (int i = 0; i < 1000; i++) {
                memset(tmp, 0, 1024);
                bt::Uint32 data[256];
                for (int j = 0; j < 256; j++)
                    data[j] = QRandomGenerator::global()->generate();

                memcpy(tmp, data, 1024);
                a.encryptReplace(reinterpret_cast<bt::Uint8 *>(data), 1024);
                b.decrypt(reinterpret_cast<bt::Uint8 *>(data), 1024);
                QVERIFY(memcmp(tmp, data, 1024) == 0);
            }
        } catch (bt::Error &err) {
            std::cout << "bt::Error caught: " << err.toString().toStdString() << std::endl;
            throw;
        }
    }
};

QTEST_MAIN(RC4EncryptorTest)

#include "rc4encryptortest.moc"
