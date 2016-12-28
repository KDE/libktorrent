/***************************************************************************
 *   Copyright (C) 2012 by                                                 *
 *   Joris Guisson <joris.guisson@gmail.com>                               *
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
#include <algorithm>
#include <QtTest>
#include <QString>
#include <util/log.h>
#include <util/error.h>
#include <dht/key.h>

using namespace bt;

static bt::Uint8 HexCharToUint8(char c)
{
	if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	else if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');
	else if (c >= '0' && c <= '9')
		return c - '0';
	else
		return 0;
}

static dht::Key KeyFromHexString(const QString & str)
{
	bt::Uint8 result[20];
	std::fill(result, result + 20, 0);

	QString s = str.toLower();
	if (s.size() % 2 != 0)
		s.prepend('0');

	int j = 19;

	for (int i = s.size() - 1; i >= 0; i -= 2)
	{
		char left = s[i - 1].toLatin1();
		char right = s[i].toLatin1();
		result[j--] = (HexCharToUint8(left) << 4) | HexCharToUint8(right);
	}

	return dht::Key(result);
}

class KeyTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{
		bt::InitLog("keytest.log", false, true);
	}

	void cleanupTestCase()
	{
	}

	void testAddition()
	{
		dht::Key a = KeyFromHexString("14455");
		dht::Key b = KeyFromHexString("FFEEDD");
		dht::Key c = a + b;
		QVERIFY(c == KeyFromHexString("1013332"));

		dht::Key d = a + 1;
		QVERIFY(d == KeyFromHexString("14456"));

		dht::Key e = KeyFromHexString("FFFF") + 1;
		QVERIFY(e == KeyFromHexString("10000"));
	}

	void testSubtraction()
	{
		dht::Key a = KeyFromHexString("556677");
		dht::Key b = KeyFromHexString("3384E6");
		dht::Key c = a - b;
		QVERIFY(c == KeyFromHexString("21E191"));

		dht::Key d = KeyFromHexString("550077");
		dht::Key e = KeyFromHexString("3384E6");
		dht::Key f = d - e;
		QVERIFY(f == KeyFromHexString("217B91"));
	}

	void testDivision()
	{
		dht::Key a = KeyFromHexString("550078");
		dht::Key b = a / 2;
		QVERIFY(b == KeyFromHexString("2A803C"));
	}
};

QTEST_MAIN(KeyTest)

#include "keytest.moc"
