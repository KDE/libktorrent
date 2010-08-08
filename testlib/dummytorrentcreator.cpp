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

#include "dummytorrentcreator.h"
#include <QFile>
#include <util/log.h>
#include <util/fileops.h>
#include <util/error.h>
#include <util/functions.h>
#include <torrent/torrentcreator.h>

using namespace bt;

DummyTorrentCreator::DummyTorrentCreator()
{
	chunk_size = 256;
	trackers.append("http://localhost:5000/announce");
	tmpdir.setAutoRemove(true);
}

DummyTorrentCreator::~DummyTorrentCreator()
{
}

bool DummyTorrentCreator::createMultiFileTorrent(const QMap<QString,bt::Uint64>& files, const QString& name)
{
	if (tmpdir.status() != 0)
		return false;
	
	try
	{
		dpath = tmpdir.name() + "data" + bt::DirSeparator() + name + bt::DirSeparator();
		bt::MakePath(dpath);
		
		QMap<QString,bt::Uint64>::const_iterator i = files.begin();
		while (i != files.end())
		{
			MakeFilePath(dpath + i.key());
			if (!createRandomFile(dpath + i.key(),i.value()))
				return false;
			i++;
		}
		
		bt::TorrentCreator creator(dpath,trackers,KUrl::List(),chunk_size,name,"",false,false);
		// Start the hashing thread and wait until it is done
		creator.start();
		creator.wait();
		creator.saveTorrent(torrentPath());
		return true;
	}
	catch (bt::Error & err)
	{
		Out(SYS_GEN|LOG_NOTICE) << "Error creating torrent: " << err.toString() << endl;
		return false;
	}
}

bool DummyTorrentCreator::createSingleFileTorrent(bt::Uint64 size, const QString& filename)
{
	if (tmpdir.status() != 0)
		return false;
	
	try
	{
		bt::MakePath(tmpdir.name() + "data" + bt::DirSeparator());
		dpath = tmpdir.name() + "data" + bt::DirSeparator() + filename;
		if (!createRandomFile(dpath,size))
			return false;
		
		bt::TorrentCreator creator(dpath,trackers,KUrl::List(),chunk_size,filename,"",false,false);
		// Start the hashing thread and wait until it is done
		creator.start();
		creator.wait();
		creator.saveTorrent(torrentPath());
		return true;
	}
	catch (bt::Error & err)
	{
		Out(SYS_GEN|LOG_NOTICE) << "Error creating torrent: " << err.toString() << endl;
		return false;
	}
}

bool DummyTorrentCreator::createRandomFile(const QString& path, bt::Uint64 size)
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
	{
		Out(SYS_GEN|LOG_NOTICE) << "Error opening " << path << ": " << file.errorString() << endl;
		return false;
	}
	
	
	bt::Uint64 written = 0;
	while (written < size)
	{
		char tmp[4096];
		for (int i = 0;i < 4096;i++)
			tmp[i] = rand() % 0xFF;
		
		bt::Uint64 to_write = 4096;
		if (to_write + written > size)
			to_write = size - written;
		written += file.write(tmp,to_write);
	}
	
	return true;
}


QString DummyTorrentCreator::dataPath() const
{
	return dpath;
}

QString DummyTorrentCreator::torrentPath() const
{
	return tmpdir.name() + "torrent";
}
