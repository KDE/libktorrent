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

#ifndef BT_EXTRACTFILEJOB_H
#define BT_EXTRACTFILEJOB_H

#include <kio/jobclasses.h>
#include <ktorrent_export.h>
#include <karchive.h>


namespace bt 
{
	class ExtractFileThread;
	
	/**
		Job which extracts a single file out of an archive
	*/
	class KTORRENT_EXPORT ExtractFileJob : public KIO::Job
	{
		Q_OBJECT
	public:
		ExtractFileJob(KArchive* archive,const QString & path,const QString & dest);
		~ExtractFileJob() override;
		
		void start() override;
		virtual void kill(bool quietly=true);
	private Q_SLOTS:
		void extractThreadDone();
		
	private:
		KArchive* archive;
		QString path;
		QString dest;
		ExtractFileThread* extract_thread;
	};

}

#endif // BT_EXTRACTFILEJOB_H
