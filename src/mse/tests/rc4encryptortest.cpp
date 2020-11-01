/***************************************************************************
 *   Copyright (C) 2010 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include <QtTest>
#include <QObject>
#include <dht/key.h>
#include <time.h>
#include <mse/rc4encryptor.h>


class RC4EncryptorTest : public QObject
{
	Q_OBJECT
public:
	
	bt::SHA1Hash randomKey()
	{
		bt::Uint32 hash[5];
		for (int i = 0; i < 5; i++)
		{
			hash[i] = QRandomGenerator::global()->generate();
		}
		return bt::SHA1Hash(reinterpret_cast<Uint8*>(hash));
	}
	
private Q_SLOTS:
	void initTestCase()
	{
	}
	
	void cleanupTestCase()
	{
	}
	
	void testRC4()
	{
		bt::SHA1Hash dkey = randomKey();
		bt::SHA1Hash ekey = randomKey();
		mse::RC4Encryptor a(dkey,ekey);
		mse::RC4Encryptor b(ekey,dkey);
		
		bt::Uint8 tmp[1024];
		for (int i = 0;i < 1000;i++)
		{
			memset(tmp,0,1024);
			bt::Uint32 data[256];
			for (int j = 0;j < 256;j++)
				data[j] = QRandomGenerator::global()->generate();
			
			memcpy(tmp,data,1024);
			a.encryptReplace(reinterpret_cast<Uint8*>(data),1024);
			b.decrypt(reinterpret_cast<Uint8*>(data),1024);
			QVERIFY(memcmp(tmp,data,1024) == 0);
		}
	}
};

QTEST_MAIN(RC4EncryptorTest)

#include "rc4encryptortest.moc"
