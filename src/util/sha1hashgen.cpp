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
#include "sha1hashgen.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <QtCrypto>
#include "functions.h"



namespace bt
{
	static QCA::Initializer qca_init;

	SHA1HashGen::SHA1HashGen() : h(new QCA::Hash(QStringLiteral("sha1")))
	{
		memset(result,9,20);
	}


	SHA1HashGen::~SHA1HashGen()
	{
		delete h;
	}

	SHA1Hash SHA1HashGen::generate(const Uint8* data,Uint32 len)
	{
		h->update((const char*)data,len);
		return SHA1Hash((const bt::Uint8*)h->final().constData());
	}
	
	void SHA1HashGen::start()
	{
		h->clear();
	}
		
	void SHA1HashGen::update(const Uint8* data,Uint32 len)
	{
		h->update((const char*)data,len);
	}
		
	 
	void SHA1HashGen::end()
	{
		memcpy(result,h->final().constData(),20);
	}
		

	SHA1Hash SHA1HashGen::get() const
	{
		return SHA1Hash(result);
	}
}
