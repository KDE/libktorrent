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
#include "httptracker.h"
#include <config-ktorrent.h>

#include <QHostAddress>
#include <util/log.h>
#include <util/functions.h>
#include <util/error.h>
#include <util/waitjob.h>
#include <interfaces/exitoperation.h>
#include <interfaces/torrentinterface.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kio/scheduler.h>
#include <klocalizedstring.h>
#include <kprotocolmanager.h>
#include <bcodec/bnode.h>
#include <bcodec/bdecoder.h>
#include <peer/peermanager.h>
#include <torrent/server.h>
#include <torrent/globals.h>
#include "version.h"
#include "httpannouncejob.h"
#include "kioannouncejob.h"


namespace bt
{
	bool HTTPTracker::proxy_on = false;
	QString HTTPTracker::proxy = QString();
	Uint16 HTTPTracker::proxy_port = 8080;
	bool HTTPTracker::use_qhttp = false;

	HTTPTracker::HTTPTracker(const QUrl & url, TrackerDataSource* tds, const PeerID & id, int tier)
			: Tracker(url, tds, id, tier),
			supports_partial_seed_extension(false)
	{
		active_job = 0;

		interval = 5 * 60; // default interval 5 minutes
		failures = 0;
		connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
	}

	HTTPTracker::~HTTPTracker()
	{
	}

	void HTTPTracker::start()
	{
		event = QStringLiteral("started");
		resetTrackerStats();
		doRequest();
	}

	void HTTPTracker::stop(WaitJob* wjob)
	{
		if (!started)
		{
			announce_queue.clear();
			reannounce_timer.stop();
			if (active_job)
			{
				active_job->kill();
				active_job = 0;
				status = TRACKER_IDLE;
				requestOK();
			}
		}
		else
		{
			reannounce_timer.stop();
			event = QStringLiteral("stopped");
			doRequest(wjob);
			started = false;
		}
	}

	void HTTPTracker::completed()
	{
		event = QStringLiteral("completed");
		doRequest();
		event = QString();
	}

	void HTTPTracker::manualUpdate()
	{
		if (!started)
			start();
		else
			doRequest();
	}

	void HTTPTracker::scrape()
	{
		if (!url.isValid())
		{
			Out(SYS_TRK | LOG_NOTICE) << "Invalid tracker url, canceling scrape" << endl;
			return;
		}

		if (!url.fileName().startsWith(QLatin1String("announce")))
		{
			Out(SYS_TRK | LOG_NOTICE) << "Tracker " << url << " does not support scraping" << endl;
			return;
		}

		QUrl scrape_url = url;
		scrape_url.setPath(url.path(QUrl::FullyEncoded).replace(QStringLiteral("announce"), QStringLiteral("scrape")), QUrl::StrictMode);

		QString epq = scrape_url.query(QUrl::FullyEncoded);
		const SHA1Hash & info_hash = tds->infoHash();
		if (scrape_url.queryItems().count() > 0)
			epq += QLatin1String("&info_hash=") + info_hash.toURLString();
		else
			epq += QLatin1String("?info_hash=") + info_hash.toURLString();
		scrape_url.setQuery(epq, QUrl::StrictMode);

		Out(SYS_TRK | LOG_NOTICE) << "Doing scrape request to url : " << scrape_url << endl;
		KIO::MetaData md;
		setupMetaData(md);

		KIO::StoredTransferJob* j = KIO::storedGet(scrape_url, KIO::NoReload, KIO::HideProgressInfo);
		// set the meta data
		j->setMetaData(md);
		KIO::Scheduler::scheduleJob(j);

		connect(j, SIGNAL(result(KJob*)), this, SLOT(onScrapeResult(KJob*)));
	}

	void HTTPTracker::onScrapeResult(KJob* j)
	{
		if (j->error())
		{
			Out(SYS_TRK | LOG_IMPORTANT) << "Scrape failed : " << j->errorString() << endl;
			return;
		}

		KIO::StoredTransferJob* st = (KIO::StoredTransferJob*)j;
		BDecoder dec(st->data(), false, 0);
		QScopedPointer<BNode> n;

		try
		{
			n.reset(dec.decode());
		}
		catch (bt::Error & err)
		{
			Out(SYS_TRK | LOG_IMPORTANT) << "Invalid scrape data " << err.toString() << endl;
			return;
		}

		if (n && n->getType() == BNode::DICT)
		{
			BDictNode* d = (BDictNode*)n.data();
			d = d->getDict(QByteArrayLiteral("files"));
			if (d)
			{
				d = d->getDict(tds->infoHash().toByteArray());
				if (d)
				{
					try
					{
						seeders = d->getInt(QByteArrayLiteral("complete"));
						leechers = d->getInt(QByteArrayLiteral("incomplete"));
						total_downloaded = d->getInt(QByteArrayLiteral("downloaded"));
						supports_partial_seed_extension = d->getValue(QByteArrayLiteral("downloaders")) != 0;
						Out(SYS_TRK | LOG_DEBUG) << "Scrape : leechers = " << leechers
						<< ", seeders = " << seeders << ", downloaded = " << total_downloaded << endl;
					}
					catch (...)
						{}
					scrapeDone();
					if (status == bt::TRACKER_ERROR)
					{
						status = bt::TRACKER_OK;
						failures = 0;
					}
				}
			}
		}
	}

	void HTTPTracker::doRequest(WaitJob* wjob)
	{
		if (!url.isValid())
		{
			requestPending();
			QTimer::singleShot(500, this, SLOT(emitInvalidURLFailure()));
			return;
		}

		Uint16 port = ServerInterface::getPort();

		QUrlQuery query(url);
		query.addQueryItem(QStringLiteral("peer_id"), peer_id.toString());
		query.addQueryItem(QStringLiteral("port"), QString::number(port));
		query.addQueryItem(QStringLiteral("uploaded"), QString::number(bytesUploaded()));
		query.addQueryItem(QStringLiteral("downloaded"), QString::number(bytesDownloaded()));

		if (event == QLatin1String("completed"))
			query.addQueryItem(QStringLiteral("left"), QStringLiteral("0")); // need to send 0 when we are completed
		else
			query.addQueryItem(QStringLiteral("left"), QString::number(tds->bytesLeft()));

		query.addQueryItem(QStringLiteral("compact"), QStringLiteral("1"));
		if (event != QLatin1String("stopped"))
			query.addQueryItem(QStringLiteral("numwant"), QStringLiteral("200"));
		else
			query.addQueryItem(QStringLiteral("numwant"), QStringLiteral("0"));

		query.addQueryItem(QStringLiteral("key"), QString::number(key));
		QString cip = Tracker::getCustomIP();
		if (cip.isNull())
			cip = CurrentIPv6Address();
		
		if (!cip.isEmpty())
			query.addQueryItem(QStringLiteral("ip"), cip);
		
		if (event.isEmpty() && supports_partial_seed_extension && tds->isPartialSeed())
			event = QStringLiteral("paused");

		if (!event.isEmpty())
			query.addQueryItem(QStringLiteral("event"), event);
		
		const SHA1Hash & info_hash = tds->infoHash();
		QString epq = query.toString(QUrl::FullyEncoded) + QLatin1String("&info_hash=") + info_hash.toURLString();

		QUrl u = url;
		u.setQuery(epq, QUrl::StrictMode);

		if (active_job)
		{
			announce_queue.append(u);
			Out(SYS_TRK | LOG_NOTICE) << "Announce ongoing, queueing announce" << endl;
		}
		else
		{
			doAnnounce(u);
			// if there is a wait job, add this job to the waitjob
			if (wjob)
				wjob->addExitOperation(new ExitJobOperation(active_job));
		}
	}

	bool HTTPTracker::updateData(const QByteArray & data)
	{
//#define DEBUG_PRINT_RESPONSE
#ifdef DEBUG_PRINT_RESPONSE
		Out(SYS_TRK | LOG_DEBUG) << "Data : " << endl;
		Out(SYS_TRK | LOG_DEBUG) << QString(data) << endl;
#endif
		// search for dictionary, there might be random garbage infront of the data
		int i = 0;
		while (i < data.size())
		{
			if (data[i] == 'd')
				break;
			i++;
		}

		if (i == data.size())
		{
			failures++;
			failed(i18n("Invalid response from tracker"));
			return false;
		}

		BDecoder dec(data, false, i);
		BNode* n = 0;
		try
		{
			n = dec.decode();
		}
		catch (...)
		{
			failures++;
			failed(i18n("Invalid data from tracker"));
			return false;
		}

		if (!n || n->getType() != BNode::DICT)
		{
			failures++;
			failed(i18n("Invalid response from tracker"));
			return false;
		}

		BDictNode* dict = (BDictNode*)n;
		if (dict->getData(QByteArrayLiteral("failure reason")))
		{
			BValueNode* vn = dict->getValue(QByteArrayLiteral("failure reason"));
			error = vn->data().toString();
			delete n;
			failures++;
			failed(error);
			return false;
		}

		if (dict->getData(QByteArrayLiteral("warning message")))
		{
			BValueNode* vn = dict->getValue(QByteArrayLiteral("warning message"));
			warning = vn->data().toString();
		}
		else
			warning.clear();

		BValueNode* vn = dict->getValue(QByteArrayLiteral("interval"));

		// if no interval is specified, use 5 minutes
		if (vn)
			interval = vn->data().toInt();
		else
			interval = 5 * 60;

		vn = dict->getValue(QByteArrayLiteral("incomplete"));
		if (vn)
			leechers = vn->data().toInt();

		vn = dict->getValue(QByteArrayLiteral("complete"));
		if (vn)
			seeders = vn->data().toInt();

		BListNode* ln = dict->getList(QByteArrayLiteral("peers"));
		if (!ln)
		{
			// no list, it might however be a compact response
			vn = dict->getValue(QByteArrayLiteral("peers"));
			if (vn && vn->data().getType() == Value::STRING)
			{
				QByteArray arr = vn->data().toByteArray();
				for (int i = 0;i < arr.size();i += 6)
				{
					Uint8 buf[6];
					for (int j = 0;j < 6;j++)
						buf[j] = arr[i + j];

					Uint32 ip = ReadUint32(buf, 0);
					addPeer(net::Address(ip, ReadUint16(buf, 4)), false);
				}
			}
		}
		else
		{
			for (Uint32 i = 0;i < ln->getNumChildren();i++)
			{
				BDictNode* dict = dynamic_cast<BDictNode*>(ln->getChild(i));

				if (!dict)
					continue;

				BValueNode* ip_node = dict->getValue(QByteArrayLiteral("ip"));
				BValueNode* port_node = dict->getValue(QByteArrayLiteral("port"));

				if (!ip_node || !port_node)
					continue;

				net::Address addr(ip_node->data().toString(), port_node->data().toInt());
				addPeer(addr, false);
			}
		}

		// Check for IPv6 compact peers
		vn = dict->getValue(QByteArrayLiteral("peers6"));
		if (vn && vn->data().getType() == Value::STRING)
		{
			QByteArray arr = vn->data().toByteArray();
			for (int i = 0;i < arr.size();i += 18)
			{
				Q_IPV6ADDR ip;
				memcpy(ip.c, arr.data() + i, 16);
				quint16 port = ReadUint16((const Uint8*)arr.data() + i, 16);

				addPeer(net::Address(ip, port), false);
			}
		}

		delete n;
		return true;
	}

	void HTTPTracker::onKIOAnnounceResult(KJob* j)
	{
		KIOAnnounceJob* st = (KIOAnnounceJob*)j;
		onAnnounceResult(st->announceUrl(), st->replyData(), j);
	}

	void HTTPTracker::onQHttpAnnounceResult(KJob* j)
	{
		HTTPAnnounceJob* st = (HTTPAnnounceJob*)j;
		onAnnounceResult(st->announceUrl(), st->replyData(), j);
	}

	void HTTPTracker::onAnnounceResult(const QUrl& url, const QByteArray& data, KJob* j)
	{
		timer.stop();
		active_job = 0;
		if (j->error() && data.size() == 0)
		{
			QString err = error;
			error.clear();
			if (err.isEmpty())
				err = j->errorString();

			Out(SYS_TRK | LOG_IMPORTANT) << "Error : " << err << endl;
			if (QUrlQuery(url).queryItemValue(QStringLiteral("event")) != QLatin1String("stopped"))
			{
				failures++;
				failed(err);
			}
			else
			{
				status = TRACKER_IDLE;
				stopDone();
			}
		}
		else
		{
			if (QUrlQuery(url).queryItemValue(QStringLiteral("event")) != QLatin1String("stopped"))
			{
				try
				{
					if (updateData(data))
					{
						failures = 0;
						peersReady(this);
						request_time = QDateTime::currentDateTime();
						status = TRACKER_OK;
						if (QUrlQuery(url).queryItemValue(QStringLiteral("event")) == QLatin1String("started"))
							started = true;
						if (started)
							reannounce_timer.start(interval * 1000);
						requestOK();
					}
				}
				catch (bt::Error & err)
				{
					failures++;
					failed(i18n("Invalid response from tracker"));
				}
				event = QString();
			}
			else
			{
				status = TRACKER_IDLE;
				failures = 0;
				stopDone();
			}
		}
		doAnnounceQueue();
	}

	void HTTPTracker::emitInvalidURLFailure()
	{
		failures++;
		failed(i18n("Invalid tracker URL"));
	}

	void HTTPTracker::setupMetaData(KIO::MetaData & md)
	{
		md["UserAgent"] = bt::GetVersionString();
		md["SendLanguageSettings"] = "false";
		md["cookies"] = "none";
		//	md["accept"] = "text/plain";
		md["accept"] = "text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2";
		if (proxy_on)
		{
			QString p = QString("%1:%2").arg(proxy).arg(proxy_port);
			if (!p.startsWith("http://"))
				p = "http://" + p;
			// set the proxy if the doNotUseKDEProxy ix enabled (URL must be valid to)
			QUrl url(p);
			if (url.isValid() && proxy.trimmed().length() >  0)
				md["UseProxy"] = p;
			else
				md["UseProxy"] = QString();

			Out(SYS_TRK | LOG_DEBUG) << "Using proxy : " << md["UseProxy"] << endl;
		}
	}

	void HTTPTracker::doAnnounceQueue()
	{
		if (announce_queue.empty())
			return;

		QUrl u = announce_queue.front();
		announce_queue.pop_front();
		doAnnounce(u);
	}

	void HTTPTracker::doAnnounce(const QUrl & u)
	{
		Out(SYS_TRK | LOG_NOTICE) << "Doing tracker request to url (via " << (use_qhttp ? "QHttp" : "KIO") << "): " << u.toString() << endl;

		if (!use_qhttp)
		{
			KIO::MetaData md;
			setupMetaData(md);
			KIOAnnounceJob* j = new KIOAnnounceJob(u, md);
			connect(j, SIGNAL(result(KJob*)), this, SLOT(onKIOAnnounceResult(KJob*)));
			active_job = j;
		}
		else
		{
			HTTPAnnounceJob* j = new HTTPAnnounceJob(u);
			connect(j, SIGNAL(result(KJob*)), this, SLOT(onQHttpAnnounceResult(KJob*)));
			if (!proxy_on)
			{
				QString proxy = KProtocolManager::proxyForUrl(u); // Use KDE settings
				if (!proxy.isNull() && proxy != QLatin1String("DIRECT"))
				{
					QUrl proxy_url(proxy);
					j->setProxy(proxy_url.host(), proxy_url.port() <= 0 ? 80 : proxy_url.port());
				}
			}
			else if (!proxy.isNull())
			{
				j->setProxy(proxy, proxy_port);
			}
			active_job = j;
			j->start();
		}

		timer.start(60*1000);
		status = TRACKER_ANNOUNCING;
		requestPending();
	}

	void HTTPTracker::onTimeout()
	{
		if (active_job)
		{
			error = i18n("Timeout contacting tracker %1", url.toString());
			active_job->kill(KJob::EmitResult);
		}
	}


	void HTTPTracker::setProxy(const QString & p, const bt::Uint16 port)
	{
		proxy = p;
		proxy_port = port;
	}

	void HTTPTracker::setProxyEnabled(bool on)
	{
		proxy_on = on;
	}


	void HTTPTracker::setUseQHttp(bool on)
	{
		use_qhttp = false;
	}

}
