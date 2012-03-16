/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include "key.h"
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <util/constants.h>

using namespace bt;

namespace dht
{

	Key::Key()
	{}

	Key::Key(const bt::SHA1Hash & k) : bt::SHA1Hash(k)
	{
	}

	Key::Key(const Uint8* d) : bt::SHA1Hash(d)
	{
	}

	Key::Key(const QByteArray & ba)
	{
		for (int i = 0;i < 20 && i < ba.size();i++)
			hash[i] = ba[i];
	}

	Key::~Key()
	{}

	bool Key::operator == (const Key & other) const
	{
		return bt::SHA1Hash::operator ==(other);
	}

	bool Key::operator != (const Key & other) const
	{
		return !operator == (other);
	}

	bool Key::operator < (const Key & other) const
	{
		for (int i = 0;i < 20;i++)
		{
			if (hash[i] < other.hash[i])
				return true;
			else if (hash[i] > other.hash[i])
				return false;
		}
		return false;
	}

	bool Key::operator <= (const Key & other) const
	{
		return operator < (other) || operator == (other);
	}

	bool Key::operator > (const Key & other) const
	{
		for (int i = 0;i < 20;i++)
		{
			if (hash[i] < other.hash[i])
				return false;
			else if (hash[i] > other.hash[i])
				return true;
		}
		return false;
	}

	bool Key::operator >= (const Key & other) const
	{
		return operator > (other) || operator == (other);
	}
	
	Key operator + (const dht::Key& a, const dht::Key& b)
	{
		bt::Uint8 result[20];
		bool carry = false;
		const bt::Uint8* ad = a.getData();
		const bt::Uint8* bd = b.getData();
		for (int i = 19;i >= 0;i--)
		{
			unsigned int r = ad[i] + bd[i] + (carry ? 1 : 0);
			if (r > 255)
			{
				result[i] = r & 0xFF;
				carry = true;
			}
			else
			{
				result[i] = r;
				carry = false;
			}
		}
		
		return dht::Key(result);
	}
	
	Key operator + (const Key & a, bt::Uint8 value)
	{
		bt::Uint8 b[20];
		memset(b, 0, 20);
		b[19] = value;
		return a + Key(b);
	}

	Key Key::operator / (int value) const
	{
		bt::Uint8 result[20];
		bt::Uint8 remainder = 0;
		
		for (int i = 0; i < 20; i++)
		{
			bt::Uint8 d = (hash[i] + (remainder << 8)) / value;
			remainder = (hash[i] + (remainder << 8)) % value;
			result[i] = d;
		}
		
		return dht::Key(result);
	}


	Key Key::distance(const Key & a, const Key & b)
	{
		return a ^ b;
	}

	Key Key::random()
	{
		srand(time(0));
		Key k;
		for (int i = 0;i < 20;i++)
		{
			k.hash[i] = (Uint8)rand() % 0xFF;
		}
		return k;
	}

	Key operator - (const Key & a, const Key & b)
	{
		bt::Uint8 result[20];
		bool carry = false;
		const bt::Uint8* ad = a.getData();
		const bt::Uint8* bd = b.getData();
		for (int i = 19;i >= 0;i--)
		{
			if (ad[i] >= bd[i])
			{
				result[i] = ad[i] - bd[i];
				if (carry)
				{
					if (result[i] == 0)
					{
						result[i] = 0xFF;
						carry = true;
					}
					else
					{
						result[i] -= 1;
						carry = false;
					}
				}
			}
			else
			{
				result[i] = 256 - (bd[i] - ad[i]);
				if (carry)
					result[i] -= 1;
				carry = true;
			}
		}
		
		return dht::Key(result);
	}

	Key Key::mid(const dht::Key& a, const dht::Key& b)
	{
		if (a <= b)
			return a + (b - a) / 2;
		else
			return b + (a - b) / 2;
	}
	
	Key Key::max()
	{
		bt::Uint8 result[20];
		std::fill(result, result + 20, 0xFF);
		return Key(result);
	}

	Key Key::min()
	{
		bt::Uint8 result[20];
		std::fill(result, result + 20, 0x0);
		return Key(result);
	}


}
