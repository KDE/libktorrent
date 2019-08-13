/***************************************************************************
 *   Copyright (C) 2012 by Joris Guisson                                   *
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

#ifndef DHT_ERRMSG_H
#define DHT_ERRMSG_H

#include "rpcmsg.h"

namespace dht
{

	/**
	 * Error message in the DHT protocol
	 */
	class KTORRENT_EXPORT ErrMsg : public RPCMsg
	{
	public:
		ErrMsg();
		ErrMsg(const QByteArray& mtid, const dht::Key& id, const QString& msg);
		~ErrMsg() override;

		void apply(DHT* dh_table) override;
		void print() override;
		void encode(QByteArray & arr) const override;
		void parse(bt::BDictNode* dict) override;
		
		/// Get the error message
		const QString & message() const {return msg;}

		typedef QSharedPointer<ErrMsg> Ptr;
	private:
		QString msg;
	};
}

#endif // DHT_ERRMSG_H
