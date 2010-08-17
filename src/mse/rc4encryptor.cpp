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
#include "rc4encryptor.h"

namespace mse
{

	
	static Uint8 rc4_enc_buffer[bt::MAX_MSGLEN];
	
	RC4Encryptor::RC4Encryptor(const bt::SHA1Hash & dk,const bt::SHA1Hash & ek) 
	{
		RC4_set_key(&enc_key,20,ek.getData());
		RC4_set_key(&dec_key,20,dk.getData());
		
		Uint8 tmp[1024];
		RC4(&enc_key,1024,tmp,tmp);
		RC4(&dec_key,1024,tmp,tmp);
	}


	RC4Encryptor::~RC4Encryptor()
	{}


	void RC4Encryptor::decrypt(Uint8* data,Uint32 len)
	{
		RC4(&dec_key,len,data,data);
	}

	const Uint8* RC4Encryptor::encrypt(const Uint8* data,Uint32 len)
	{
		RC4(&enc_key,len,data,rc4_enc_buffer);
		return rc4_enc_buffer;
	}
	
	void RC4Encryptor::encryptReplace(Uint8* data,Uint32 len)
	{
		RC4(&enc_key,len,data,data);
	}
	
}
