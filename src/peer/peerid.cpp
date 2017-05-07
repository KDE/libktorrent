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
#include "peerid.h"
#include <time.h>
#include <stdlib.h>
#include <qmap.h>
#include <klocalizedstring.h>
#include "version.h"

namespace bt
{
	char RandomLetterOrNumber()
	{
		int i = rand() % 62;
		if (i < 26)
			return 'a' + i;
		else if (i < 52)
			return 'A' + (i - 26);
		else
			return '0' + (i - 52);
	}

	QString charToNumber(QChar c)
	{
		if (c >= 'A' && c <= 'F')
			return QString::number(c.toLatin1() - 'A' + 10);
		else if (c >= 'z' && c <= 'f')
			return QString::number(c.toLatin1() - 'a' + 10);
		else
			return c;
	}

	
	PeerID::PeerID()
	{
		srand(time(0));
		memcpy(id,bt::PeerIDPrefix().toLatin1(),8);
		for (int i = 8;i < 20;i++)
			id[i] = RandomLetterOrNumber(); 
		client_name = identifyClient();
	}

	PeerID::PeerID(const char* pid)
	{
		if (pid)
			memcpy(id,pid,20);
		else
			memset(id,0,20);
		client_name = identifyClient();
	}

	PeerID::PeerID(const PeerID & pid)
	{
		memcpy(id,pid.id,20);
		client_name = pid.client_name;
	}

	PeerID::~PeerID()
	{}



	PeerID & PeerID::operator = (const PeerID & pid)
	{
		memcpy(id,pid.id,20);
		client_name = pid.client_name;
		return *this;
	}

	bool operator == (const PeerID & a,const PeerID & b)
	{
		for (int i = 0;i < 20;i++)
			if (a.id[i] != b.id[i])
				return false;

		return true;
	}

	bool operator != (const PeerID & a,const PeerID & b)
	{
		return ! operator == (a,b);
	}

	bool operator < (const PeerID & a,const PeerID & b)
	{
		for (int i = 0;i < 20;i++)
			if (a.id[i] < b.id[i])
				return true;

		return false;
	}

	QString PeerID::toString() const
	{
		QString r; r.reserve(20);
		for (int i = 0;i < 20;i++)
			r += id[i] == 0 ? ' ' : id[i];
		return r;
	}

	QString PeerID::identifyClient() const
	{
		if (!client_name.isNull())
			return client_name;
		
		QString peer_id = toString();
		// we only need to create this map once
		// so make it static
		static QMap<QString, QString> Map;
		static bool first = true; 

		if (first)
		{
			// Keep things a bit alphabetic to make it easier add new ones
			//AZUREUS STYLE
			Map[QStringLiteral("7T")] = QStringLiteral("aTorrent");
			Map[QStringLiteral("AB")] = QStringLiteral("AnyEvent::BitTorrent");
			Map[QStringLiteral("AG")] = QStringLiteral("Ares");
			Map[QStringLiteral("AR")] = QStringLiteral("Arctic");
			Map[QStringLiteral("AT")] = QStringLiteral("Artemis");
			Map[QStringLiteral("AV")] = QStringLiteral("Avicora");
			Map[QStringLiteral("AX")] = QStringLiteral("BitPump");
			Map[QStringLiteral("AZ")] = QStringLiteral("Azureus");
			Map[QStringLiteral("A~")] = QStringLiteral("Ares");
			Map[QStringLiteral("BB")] = QStringLiteral("BitBuddy");
			Map[QStringLiteral("BC")] = QStringLiteral("BitComet");
			Map[QStringLiteral("BE")] = QStringLiteral("Baretorrent");
			Map[QStringLiteral("BF")] = QStringLiteral("Bitflu");
			Map[QStringLiteral("BG")] = QStringLiteral("BTGetit");
			Map[QStringLiteral("BL")] = QStringLiteral("BitBlinder");
			Map[QStringLiteral("BM")] = QStringLiteral("BitMagnet");
			Map[QStringLiteral("BO")] = QStringLiteral("BitsOnWheels");
			Map[QStringLiteral("BP")] = QStringLiteral("BitTorrent Pro");
			Map[QStringLiteral("BR")] = QStringLiteral("BitRocket");
			Map[QStringLiteral("BS")] = QStringLiteral("BTSlave");
			Map[QStringLiteral("BT")] = QStringLiteral("mainline BitTorrent");
			Map[QStringLiteral("Bt")] = QStringLiteral("Bt");
			Map[QStringLiteral("BW")] = QStringLiteral("BitWombat");
			Map[QStringLiteral("BX")] = QStringLiteral("BitTorrent X");
			Map[QStringLiteral("CD")] = QStringLiteral("Enhanced CTorrent");
			Map[QStringLiteral("CS")] = QStringLiteral("SpywareTerminator");
			Map[QStringLiteral("CT")] = QStringLiteral("CTorrent");
			Map[QStringLiteral("DE")] = QStringLiteral("DelugeTorrent");
			Map[QStringLiteral("DP")] = QStringLiteral("Propagate Data Client");
			Map[QStringLiteral("EB")] = QStringLiteral("EBit");
			Map[QStringLiteral("ES")] = QStringLiteral("electric sheep");
			Map[QStringLiteral("FC")] = QStringLiteral("FileCroc");
			Map[QStringLiteral("FD")] = QStringLiteral("Free Download Manager");
			Map[QStringLiteral("FT")] = QStringLiteral("FoxTorrent");
			Map[QStringLiteral("FX")] = QStringLiteral("Freebox BitTorrent");
			Map[QStringLiteral("G3")] = QStringLiteral("G3 Torrent");
			Map[QStringLiteral("GS")] = QStringLiteral("GSTorrent");
			Map[QStringLiteral("HK")] = QStringLiteral("Hekate");
			Map[QStringLiteral("HL")] = QStringLiteral("Halite");
			Map[QStringLiteral("HM")] = QStringLiteral("hMule");
			Map[QStringLiteral("HN")] = QStringLiteral("Hydranode");
			Map[QStringLiteral("IL")] = QStringLiteral("iLivid");
			Map[QStringLiteral("JS")] = QStringLiteral("Justseed.it client");
			Map[QStringLiteral("JT")] = QStringLiteral("JavaTorrent");
			Map[QStringLiteral("KG")] = QStringLiteral("KGet");
			Map[QStringLiteral("KT")] = QStringLiteral("KTorrent"); // lets not forget our own client
			Map[QStringLiteral("LC")] = QStringLiteral("LeechCraft");
			Map[QStringLiteral("LH")] = QStringLiteral("LH-ABC");
			Map[QStringLiteral("LP")] = QStringLiteral("Lphant");
			Map[QStringLiteral("LT")] = QStringLiteral("libtorrent");
			Map[QStringLiteral("lt")] = QStringLiteral("libTorrent");
			Map[QStringLiteral("LW")] = QStringLiteral("LimeWire");
			Map[QStringLiteral("MK")] = QStringLiteral("Meerkat");
			Map[QStringLiteral("ML")] = QStringLiteral("MLDonkey");
			Map[QStringLiteral("MO")] = QStringLiteral("MonoTorrent");
			Map[QStringLiteral("MP")] = QStringLiteral("MooPolice");
			Map[QStringLiteral("MR")] = QStringLiteral("Miro");
			Map[QStringLiteral("MT")] = QStringLiteral("MoonLight");
			Map[QStringLiteral("NB")] = QStringLiteral("Net::BitTorrent");
			Map[QStringLiteral("NE")] = QStringLiteral("BT Next Evolution");
			Map[QStringLiteral("NX")] = QStringLiteral("Net Transport");
			Map[QStringLiteral("OS")] = QStringLiteral("OneSwarm");
			Map[QStringLiteral("OT")] = QStringLiteral("OmegaTorrent");
			Map[QStringLiteral("PB")] = QStringLiteral("Protocol::BitTorrent");
			Map[QStringLiteral("PD")] = QStringLiteral("Pando");
			Map[QStringLiteral("qB")] = QStringLiteral("qBittorrent");
			Map[QStringLiteral("QD")] = QStringLiteral("QQDownload");
			Map[QStringLiteral("QT")] = QStringLiteral("Qt 4 Torrent example");
			Map[QStringLiteral("RT")] = QStringLiteral("Retriever");
			Map[QStringLiteral("RZ")] = QStringLiteral("RezTorrent");
			Map[QStringLiteral("SB")] = QStringLiteral("Swiftbit");
			Map[QStringLiteral("SD")] = QStringLiteral("Thunder");
			Map[QStringLiteral("SM")] = QStringLiteral("SoMud");
			Map[QStringLiteral("SP")] = QStringLiteral("BitSpirit");
			Map[QStringLiteral("SS")] = QStringLiteral("SwarmScope");
			Map[QStringLiteral("st")] = QStringLiteral("sharktorrent");
			Map[QStringLiteral("ST")] = QStringLiteral("SymTorrent");
			Map[QStringLiteral("SZ")] = QStringLiteral("Shareaza");
			Map[QStringLiteral("S~")] = QStringLiteral("Shareaza alpha/beta");
			Map[QStringLiteral("TB")] = QStringLiteral("Torch");
			Map[QStringLiteral("TE")] = QStringLiteral("terasaur Seed Bank");
			Map[QStringLiteral("TL")] = QStringLiteral("Tribler");
			Map[QStringLiteral("TN")] = QStringLiteral("Torrent .NET");
			Map[QStringLiteral("TR")] = QStringLiteral("Transmission");
			Map[QStringLiteral("TS")] = QStringLiteral("Torrent Storm");
			Map[QStringLiteral("TT")] = QStringLiteral("TuoTu");
			Map[QStringLiteral("UL")] = QStringLiteral("uLeecher!");
			Map[QStringLiteral("UM")] = QStringLiteral("%1Torrent for Mac").arg(QChar(0x00B5)); // µTorrent, 0x00B5 is unicode for µ
			Map[QStringLiteral("UT")] = QStringLiteral("%1Torrent").arg(QChar(0x00B5)); // µTorrent, 0x00B5 is unicode for µ
			Map[QStringLiteral("VG")] = QStringLiteral("Vagaa");
			Map[QStringLiteral("WD")] = QStringLiteral("WebTorrent Desktop");
			Map[QStringLiteral("WT")] = QStringLiteral("BitLet");
			Map[QStringLiteral("WW")] = QStringLiteral("WebTorrent");
			Map[QStringLiteral("WY")] = QStringLiteral("FireTorrent");
			Map[QStringLiteral("XF")] = QStringLiteral("Xfplay");
			Map[QStringLiteral("XL")] = QStringLiteral("Xunlei");
			Map[QStringLiteral("XS")] = QStringLiteral("XSwifter");
			Map[QStringLiteral("XT")] = QStringLiteral("Xan Torrent");
			Map[QStringLiteral("XX")] = QStringLiteral("Xtorrent");
			Map[QStringLiteral("ZT")] = QStringLiteral("Zip Torrent");
			
			//SHADOWS STYLE
			Map[QStringLiteral("A")] = QStringLiteral("ABC");
			Map[QStringLiteral("O")] = QStringLiteral("Osprey Permaseed");
			Map[QStringLiteral("Q")] = QStringLiteral("BTQueue");
			Map[QStringLiteral("R")] = QStringLiteral("Tribler");
			Map[QStringLiteral("S")] = QStringLiteral("Shadow's");
			Map[QStringLiteral("T")] = QStringLiteral("BitTornado");
			Map[QStringLiteral("U")] = QStringLiteral("UPnP NAT BitTorrent");
			//OTHER
			Map[QStringLiteral("Plus")] = QStringLiteral("Plus! II");
			Map[QStringLiteral("OP")] = QStringLiteral("Opera");
			Map[QStringLiteral("BOW")] = QStringLiteral("Bits on Wheels");
			Map[QStringLiteral("M")] = QStringLiteral("BitTorrent");
			Map[QStringLiteral("exbc")] = QStringLiteral("BitComet");
			Map[QStringLiteral("Mbrst")] = QStringLiteral("Burst!");
			Map[QStringLiteral("XBT")] = QStringLiteral("XBT Client");
			Map[QStringLiteral("SP")] = QStringLiteral("BitSpirit");
			Map[QStringLiteral("FG")] = QStringLiteral("FlashGet");
			Map[QStringLiteral("RS")] = QStringLiteral("Rufus");
			Map[QStringLiteral("Q1")] = QStringLiteral("Queen Bee");
			Map[QStringLiteral("AP")] = QStringLiteral("AllPeers");
			Map[QStringLiteral("QVOD")] = QStringLiteral("Qvod");
			Map[QStringLiteral("TIX")] = QStringLiteral("Tixati");
			first = false;
		}

		QString name;
		if (peer_id.at(0) == '-' &&
			peer_id.at(1).isLetter() &&
			peer_id.at(2).isLetter() ) //AZ style
		{
			QString ID(peer_id.mid(1,2));
			if (ID == QLatin1String("ML"))
			{
				name = Map[ID];
				const int dash_pos = peer_id.indexOf('-', 3);
				if (dash_pos != -1)
					name += ' ' + peer_id.mid(3,dash_pos-3);
			}
			else if (Map.contains(ID))
				name = Map[ID] + ' ' + charToNumber(peer_id.at(3)) + '.' + charToNumber(peer_id.at(4)) + '.'
					+ charToNumber(peer_id.at(5));
		}
		else if (peer_id.at(0).isLetter() &&
				peer_id.at(1).isDigit() &&
				peer_id.at(2).isDigit() )  //Shadow's style
		{
			QString ID = QString(peer_id.at(0));
			if (Map.contains(ID))
				name = Map[ID] + ' ' + peer_id.at(1) + '.' +
						peer_id.at(2) + '.' + peer_id.at(3);
		}
		else if (peer_id.at(0) == 'M' && peer_id.at(2) == '-' && (peer_id.at(4) == '-' || peer_id.at(5) == '-'))
		{
			name = Map[QStringLiteral("M")] + ' ' + peer_id.at(1) + '.' + peer_id.at(3);
			if(peer_id.at(4) == '-')
				name += QStringLiteral(".%1").arg(peer_id.at(5));
			else
				name += QStringLiteral("%1.%2").arg(peer_id.at(4)).arg(peer_id.at(6));
		}
		else if (peer_id.startsWith(QLatin1String("OP")))
		{
			name = Map[QStringLiteral("OP")];
		}
		else if (peer_id.startsWith(QLatin1String("exbc")))
		{
			name = Map[QStringLiteral("exbc")];
		}
		else if (peer_id.mid(1,3) == QLatin1String("BOW"))
		{
			name = Map[QStringLiteral("BOW")];
		}
		else if (peer_id.startsWith(QLatin1String("Plus")))
		{
			name = Map[QStringLiteral("Plus")];
		}
		else if (peer_id.startsWith(QLatin1String("Mbrst")))
		{
			name = Map[QStringLiteral("Mbrst")] + ' ' + peer_id.at(5) + '.' + peer_id.at(7);
		}
		else if (peer_id.startsWith(QLatin1String("XBT")))
		{
			name = Map[QStringLiteral("XBT")] + ' ' + peer_id.at(3) + '.' + peer_id.at(4) + '.' + peer_id.at(5);
		}
		else if (peer_id.startsWith(QLatin1String("SP")))
		{
			name = Map[QStringLiteral("SP")] + ' ' + peer_id.at(2) + '.' + peer_id.at(3) + '.'
				+ peer_id.at(4) + '.' + peer_id.at(5);
		}
		else if (peer_id.startsWith(QLatin1String("FG")))
		{
			name = Map[QStringLiteral("FG")] + ' ' + peer_id.at(2) + '.' + peer_id.at(3) + '.'
				+ peer_id.at(4) + '.' + peer_id.at(5);
		}
		else if (peer_id.midRef(2,2) == QLatin1String("RS"))
		{
			name = Map[QStringLiteral("RS")];
		}
		else if (peer_id.startsWith(QLatin1String("Q1")))
		{
			name = Map[QStringLiteral("Q1")];
		}
		else if (peer_id.startsWith(QLatin1String("AP")))
		{
			name = Map[QStringLiteral("AP")];
		}
		else if (peer_id.startsWith(QLatin1String("QVOD")))
		{
			name = Map[QStringLiteral("QVOD")];
		}
		else if (peer_id.startsWith(QLatin1String("TIX")))
		{
			name = Map[QStringLiteral("TIX")] + ' ' + peer_id.at(3) + peer_id.at(4) + '.'
				+ peer_id.at(5) + peer_id.at(6);
		}
		if (name.isEmpty())
		{
			name = i18n("Unknown client");
		}
			
		return name;
	}
}
