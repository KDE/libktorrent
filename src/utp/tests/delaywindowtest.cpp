/***************************************************************************
*   Copyright (C) 2010 by Joris Guisson                                   *
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

#include <QtTest>
#include <QObject>
#include <time.h>
#include <boost/scoped_array.hpp>
#include <util/log.h>
#include <utp/delaywindow.h>
#include <util/functions.h>

using namespace utp;
using namespace bt;

class DelayWindowTest : public QObject
{
	Q_OBJECT
public:
	DelayWindowTest(QObject* parent = 0) : QObject(parent)
	{
	}
	
	
private slots:
	void initTestCase()
	{
		bt::InitLog("delaywindowtest.log",false,true);
		qsrand(time(0));
	}
	
	void cleanupTestCase()
	{
	}
	
	void testWindow()
	{
		bt::Uint32 base_delay = MAX_DELAY;
		DelayWindow wnd;
		
		for (int i = 0;i < 100;i++)
		{
			bt::Uint32 val = qrand() * qrand();
			Header hdr;
			hdr.timestamp_difference_microseconds = val;
			if (val < base_delay)
				base_delay = val;
			
			bt::Uint32 ret = wnd.update(&hdr,bt::Now());
			QVERIFY(ret == base_delay);
		}
	}
#if 1
	void testPerformance()
	{
		const int SAMPLE_COUNT = 2400000;
		
		boost::scoped_array<bt::Uint32> delay_samples(new bt::Uint32[SAMPLE_COUNT]);
		for (int i = 0;i < SAMPLE_COUNT;i++)
			delay_samples[i] = qrand() % 1000000;
		
		boost::scoped_array<bt::Uint32> returned_delay_new(new bt::Uint32[SAMPLE_COUNT]);
		boost::scoped_array<bt::Uint32> returned_delay_old(new bt::Uint32[SAMPLE_COUNT]);
		boost::scoped_array<bt::Uint32> returned_delay_circular(new bt::Uint32[SAMPLE_COUNT]);
		
		{
			DelayWindow wnd;
			bt::TimeStamp start = bt::Now();
			for (int i = 0;i < SAMPLE_COUNT;i++)
			{
				Header hdr;
				hdr.timestamp_difference_microseconds = delay_samples[i];
				returned_delay_new[i] = wnd.update(&hdr,i);
			}
			bt::TimeStamp duration = bt::Now() - start;
			Out(SYS_GEN|LOG_DEBUG) << "New algorithm took: " << duration << endl;
		}
	}
#endif

	void testTimeout()
	{
		DelayWindow wnd;
		
		Header hdr;
		hdr.timestamp_difference_microseconds = 1000;
		bt::TimeStamp ts = 1000;
		QVERIFY(wnd.update(&hdr,ts) == 1000);
		
		hdr.timestamp_difference_microseconds = 2000;
		QVERIFY(wnd.update(&hdr,ts + 1000) == 1000);
		
		// Now simulate timeout, oldest must get removed
		hdr.timestamp_difference_microseconds = 3000;
		QVERIFY(wnd.update(&hdr,ts + utp::DELAY_WINDOW_SIZE + 1) == 2000); 
	}
	
private:
};

QTEST_MAIN(DelayWindowTest)

#include "delaywindowtest.moc"

