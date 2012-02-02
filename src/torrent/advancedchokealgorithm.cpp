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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include "advancedchokealgorithm.h"
#include <util/functions.h>
#include <interfaces/torrentinterface.h>
#include <diskio/chunkmanager.h>
#include <peer/peer.h>
#include <peer/peermanager.h>
#include <krandom.h>


namespace bt
{


	const Uint32 OPT_SEL_INTERVAL = 30*1000; // we switch optimistic peer each 30 seconds
	const double NEWBIE_BONUS = 1.0;
	const double SNUB_PENALTY = 10.0;
	const double ONE_MB = 1024*1024;


	AdvancedChokeAlgorithm::AdvancedChokeAlgorithm()
			: ChokeAlgorithm()
	{
		last_opt_sel_time = 0;
	}


	AdvancedChokeAlgorithm::~AdvancedChokeAlgorithm()
	{}

	bool AdvancedChokeAlgorithm::calcACAScore(Peer::Ptr p, ChunkManager & cman, const TorrentStats & stats)
	{
		const PeerInterface::Stats & s = p->getStats();
		if (p->isSeeder() || s.partial_seed)
		{
			p->setACAScore(0.0);
			return false;
		}

		bool should_be_interested = false;
		// before we start calculating first check if we have piece that the peer doesn't have
		const BitSet & ours = cman.getBitSet();
		const BitSet & theirs = p->getBitSet();
		for (Uint32 i = 0;i < ours.getNumBits();i++)
		{
			if (ours.get(i) && !theirs.get(i))
			{
				should_be_interested = true;
				break;
			}
		}

		if (!should_be_interested || !p->isInterested())
		{
			// not interseted so it doesn't make sense to unchoke it
			p->setACAScore(-50.0);
			return false;
		}



		double nb = 0.0; // newbie bonus
		double cp = 0.0; // choke penalty
		double sp = s.snubbed ? SNUB_PENALTY : 0.0; // snubbing penalty
		double lb = s.local ? 10.0 : 0.0; // local peers get a bonus of 10
		double bd = s.bytes_downloaded; // bytes downloaded
		double tbd = stats.session_bytes_downloaded; // total bytes downloaded
		double ds = s.download_rate; // current download rate
		double tds = stats.download_rate; // total download speed

		// if the peer has less than 1 MB or 0.5 % of the torrent it is a newbie
		if (p->percentAvailable() < 0.5 && stats.total_bytes * p->percentAvailable() < 1024*1024)
		{
			nb = NEWBIE_BONUS;
		}

		if (p->isChoked())
		{
			cp = NEWBIE_BONUS; // cp cancels out newbie bonus
		}

		// NB + K * (BD/TBD) - CP - SP + L * (DS / TDS)
		double K = 5.0;
		double L = 5.0;
		double aca = lb + nb + (tbd > 0 ? K * (bd / tbd) : 0.0) + (tds > 0 ? L * (ds / tds) : 0.0) - cp - sp;

		p->setACAScore(aca);
		return true;
	}

	static bool ACAGreaterThan(Peer::Ptr a, Peer::Ptr b)
	{
		return a->getStats().aca_score > b->getStats().aca_score;
	}


	void AdvancedChokeAlgorithm::doChokingLeechingState(PeerManager & pman, ChunkManager & cman, const TorrentStats & stats)
	{
		QList<Peer::Ptr> ppl = pman.getPeers();
		for (QList<Peer::Ptr>::iterator i = ppl.begin();i != ppl.end();)
		{
			Peer::Ptr p = *i;
			if (!calcACAScore(p, cman, stats))
			{
				// choke seeders they do not want to download from us anyway
				p->choke();
				i = ppl.erase(i);
			}
			else
				i++;
		}

		// sort list by ACA score
		qSort(ppl.begin(), ppl.end(), ACAGreaterThan);

		doUnchoking(ppl, updateOptimisticPeer(pman, ppl));
	}

	void AdvancedChokeAlgorithm::doUnchoking(QList<Peer::Ptr> & ppl, Peer::Ptr poup)
	{
		// Get the number of upload slots
		Uint32 num_slots = Choker::getNumUploadSlots();
		// Do the choking and unchoking
		Uint32 num_unchoked = 0;
		foreach (Peer::Ptr p, ppl)
		{
			if (!poup && num_unchoked < num_slots)
			{
				p->sendUnchoke();
				num_unchoked++;
			}
			else if (num_unchoked < num_slots - 1 || p == poup)
			{
				p->sendUnchoke();
				if (p != poup)
					num_unchoked++;
			}
			else
			{
				p->choke();
			}
		}
	}

	static bool UploadRateGreaterThan(Peer::Ptr a, Peer::Ptr b)
	{
		return a->getStats().upload_rate > b->getStats().upload_rate;
	}

	void AdvancedChokeAlgorithm::doChokingSeedingState(PeerManager & pman, ChunkManager & cman, const TorrentStats & stats)
	{
		QList<Peer::Ptr> ppl = pman.getPeers();
		for (QList<Peer::Ptr>::iterator i = ppl.begin();i != ppl.end();)
		{
			Peer::Ptr p = *i;
			if (!calcACAScore(p, cman, stats))
			{
				// choke seeders they do not want to download from us anyway
				p->choke();
				i = ppl.erase(i);
			}
			else
				i++;
		}

		qSort(ppl.begin(), ppl.end(), UploadRateGreaterThan);

		doUnchoking(ppl, updateOptimisticPeer(pman, ppl));
	}

	static Uint32 FindPlannedOptimisticUnchokedPeer(const QList<Peer::Ptr> & ppl)
	{
		Uint32 num_peers = ppl.size();
		if (num_peers == 0)
			return UNDEFINED_ID;

		// find a random peer that is choked and interested
		Uint32 start = KRandom::random() % num_peers;
		Uint32 i = (start + 1) % num_peers;
		while (i != start)
		{
			Peer::Ptr p = ppl.at(i);
			if (p && p->isChoked() && p->isInterested() && !p->isSeeder() && ppl.contains(p))
				return p->getID();
			i = (i + 1) % num_peers;
		}

		// we do not expect to have 4 billion peers
		return UNDEFINED_ID;
	}

	Peer::Ptr AdvancedChokeAlgorithm::updateOptimisticPeer(PeerManager & pman, const QList<Peer::Ptr> & ppl)
	{
		// get the planned optimistic unchoked peer and change it if necessary
		Peer::Ptr poup = pman.findPeer(opt_unchoked_peer_id);
		TimeStamp now = CurrentTime();
		if (now - last_opt_sel_time > OPT_SEL_INTERVAL || !poup)
		{
			opt_unchoked_peer_id = FindPlannedOptimisticUnchokedPeer(ppl);
			last_opt_sel_time = now;
			poup = pman.findPeer(opt_unchoked_peer_id);
		}
		return poup;
	}
}
