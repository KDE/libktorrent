/***************************************************************************
 *   Copyright (C) 2009 by Joris Guisson                                   *
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

#include "magnetlink.h"
#include <QUrl>
#include <QStringList>
#include <util/log.h>
#include <util/error.h>

namespace bt
{

	MagnetLink::MagnetLink()
	{
	}

	MagnetLink::MagnetLink(const MagnetLink& mlink)
	{
		magnet_string = mlink.magnet_string;
		info_hash = mlink.info_hash;
		tracker_urls = mlink.tracker_urls;
		torrent_url = mlink.torrent_url;
		path = mlink.path;
		name = mlink.name;
	}

	MagnetLink::MagnetLink(const QString& mlink)
	{
		parse(mlink);
	}

	MagnetLink::~MagnetLink()
	{
	}

	MagnetLink& MagnetLink::operator=(const bt::MagnetLink& mlink)
	{
		magnet_string = mlink.magnet_string;
		info_hash = mlink.info_hash;
		tracker_urls = mlink.tracker_urls;
		torrent_url = mlink.torrent_url;
		path = mlink.path;
		name = mlink.name;
		return *this;
	}

	bool MagnetLink::operator==(const bt::MagnetLink& mlink) const
	{
		return info_hash == mlink.infoHash();
	}

	static QList<QUrl> GetTrackers(const QUrl &url)
	{
		QList<QUrl> result;
		const QString encoded_query = QString::fromLatin1(url.encodedQuery());
		const QString item = QLatin1String("tr=");
		if(encoded_query.length() <= 1)
			return result;

		const QStringList items = encoded_query.split(QString(QLatin1Char('&')), QString::SkipEmptyParts);
		const int len = item.length();
		for(QStringList::ConstIterator it = items.begin(); it != items.end(); ++it)
		{
			if((*it).startsWith(item))
			{
				if((*it).length() > len)
				{
					QString str = (*it).mid(len);
					str.replace(QLatin1Char('+'), QLatin1Char(' '));   // + in queries means space.
					result.push_back(QUrl::fromPercentEncoding(str.toLatin1()));
				}
			}
		}

		return result;
	}

	void MagnetLink::parse(const QString& mlink)
	{
		QUrl url(mlink);
		if(url.scheme() != QLatin1String("magnet"))
		{
			Out(SYS_GEN | LOG_NOTICE) << "Invalid protocol of magnet link "
			                          << mlink << endl;
			return;
		}

		torrent_url = QUrlQuery(url).queryItemValue(QLatin1String("to"), QUrl::FullyDecoded);
		//magnet://description-of-content.btih.HASH(-HASH)*.dht/path/file?x.pt=&x.to=

		// TODO automatically select these files and prefetches from here
		path = QUrlQuery(url).queryItemValue(QLatin1String("pt"));
		if(path.isEmpty() && url.path() != QLatin1String("/"))
		{
			// TODO find out why RemoveTrailingSlash does not work
			path = url.adjusted(QUrl::StripTrailingSlash).path().remove(QRegExp(QLatin1String("^/")));
		}

		QString xt = QUrlQuery(url).queryItemValue(QLatin1String("xt"));
		if(xt.isEmpty()
		        || !xt.startsWith(QLatin1String("urn:btih:")))
		{
			QRegExp btihHash(QLatin1String("([^\\.]+).btih"));
			if(btihHash.indexIn(url.host()) != -1)
			{
				QString primaryHash = btihHash.cap(1).split('-')[0];
				xt = QLatin1String("urn:btih:") + primaryHash;
			}
			else
			{
				Out(SYS_GEN | LOG_NOTICE) << "No hash found in magnet link "
				                          << mlink << endl;
				return;
			}
		}

		QString ih = xt.mid(9);
		if(ih.length() != 40 && ih.length() != 32)
		{
			Out(SYS_GEN | LOG_NOTICE) << "Hash has not valid length in magnet link "
			                          << mlink << endl;
			return;
		}

		try
		{
			if(ih.length() == 32)
				ih = base32ToHexString(ih);

			Uint8 hash[20];
			memset(hash, 0, 20);
			for(int i = 0; i < 20; i++)
			{
				Uint8 low = charToHex(ih[2 * i + 1]);
				Uint8 high = charToHex(ih[2 * i]);
				hash[i] = (high << 4) | low;
			}

			info_hash = SHA1Hash(hash);
			tracker_urls = GetTrackers(url);
			name = QUrlQuery(url).queryItemValue(QLatin1String("dn")).replace('+', ' ');
			magnet_string = mlink;
		}
		catch(...)
		{
			Out(SYS_GEN | LOG_NOTICE) << "Invalid magnet link " << mlink << endl;
		}
	}

	Uint8 MagnetLink::charToHex(const QChar& ch)
	{
		if(ch.isDigit())
			return ch.digitValue();

		if(!ch.isLetter())
			throw bt::Error("Invalid char");

		if(ch.isLower())
			return 10 + ch.toAscii() - 'a';
		else
			return 10 + ch.toAscii() - 'A';
	}

	QString MagnetLink::base32ToHexString(const QString &s)
	{
		Uint32 part;
		Uint32 tmp;
		QString ret;
		QChar ch;
		QString str = s.toUpper();
		// 32 base32 chars -> 40 hex chars
		// 4 base32 chars -> 5 hex chars
		for(int i = 0; i < 8; i++)
		{
			part = 0;
			for(int j = 0; j < 4; j++)
			{
				ch = str[i * 4 + j];
				if(ch.isDigit() && (ch.digitValue() < 2 || ch.digitValue() > 7))
					throw bt::Error("Invalid char");

				if(ch.isDigit())
					tmp = ch.digitValue() + 24;
				else
					tmp = ch.toAscii() - 'A';
				part = part + (tmp << 5 * (3 - j));
			}

			// part is a Uint32 with 20 bits (5 hex)
			for(int j = 0; j < 5; j++)
			{
				tmp = (part >> 4 * (4 - j)) & 0xf;
				if(tmp >= 10)
					ret.append(QChar((tmp - 10) + 'a'));
				else
					ret.append(QChar(tmp + '0'));
			}
		}
		return ret;
	}

}

