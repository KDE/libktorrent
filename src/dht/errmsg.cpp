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
#include "errmsg.h"
#include <bcodec/bnode.h>
#include <util/log.h>
#include <util/error.h>
#include "dht.h"

using namespace bt;

namespace dht
{
	ErrMsg::ErrMsg()
	{
	}

	ErrMsg::ErrMsg(const QByteArray & mtid, const Key & id, const QString & msg)
			: RPCMsg(mtid, NONE, ERR_MSG, id), msg(msg)
	{
	}

	ErrMsg::~ErrMsg()
	{
	}

	void ErrMsg::apply(dht::DHT* dh_table)
	{
		dh_table->error(*this);
	}

	void ErrMsg::print()
	{
		Out(SYS_DHT | LOG_NOTICE) << "ERR: " << mtid[0] << " " << msg << endl;
	}

	void ErrMsg::encode(QByteArray &) const
	{
	}

	void ErrMsg::parse(BDictNode* dict)
	{
		RPCMsg::parse(dict);
		BListNode* ln = dict->getList(ERR_DHT);
		if (!ln)
			throw bt::Error("Invalid error message");

		msg = ln->getString(1, 0);
	}

}
