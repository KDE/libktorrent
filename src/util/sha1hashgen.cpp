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
#include "functions.h"



namespace bt
{
	

	SHA1HashGen::SHA1HashGen()
	{
		memset(result,9,20);
		gcry_md_open(&handle,GCRY_MD_SHA1,0);
	}


	SHA1HashGen::~SHA1HashGen()
	{
		gcry_md_close(handle);
	}

	SHA1Hash SHA1HashGen::generate(const Uint8* data,Uint32 len)
	{
		gcry_md_write(handle,data,len);
		bt::Uint8* result = gcry_md_read(handle,GCRY_MD_SHA1);
		return SHA1Hash(result);
	}
	
	void SHA1HashGen::start()
	{
		gcry_md_reset(handle);
	}
		
	void SHA1HashGen::update(const Uint8* data,Uint32 len)
	{
		gcry_md_write(handle,data,len);
	}
		
	 
	void SHA1HashGen::end()
	{
		memcpy(result,gcry_md_read(handle,GCRY_MD_SHA1),20);
	}
		

	SHA1Hash SHA1HashGen::get() const
	{
		return SHA1Hash(result);
	}
}
