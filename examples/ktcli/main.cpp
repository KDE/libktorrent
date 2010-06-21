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

#include <QDir>
#include <kcmdlineargs.h>
#include <version.h>
#include <util/error.h>
#include <util/log.h>
#include <util/functions.h>
#include "ktcli.h"




using namespace bt;

int main(int argc, char** argv)
{
	QCoreApplication app(argc,argv);
	try
	{
		KCmdLineOptions options;
		options.add("+Url", ki18n("Torrent to open"));
		options.add("port <port>", ki18n("Port to use"), "6881");
		options.add("tmpdir <tmpdir>", ki18n("Port to use"), QDir::tempPath().toLocal8Bit());
		KCmdLineArgs::addCmdLineOptions(options);
		KCmdLineArgs::init(argc,argv,"ktcli","ktorrent",ki18n("ktcli"),bt::GetVersionString().toAscii());
		
		KTCLI app;
		if (!app.start())
			return -1;
		else
			return app.exec();
	}
	catch (bt::Error & err)
	{
		Out(SYS_GEN|LOG_IMPORTANT) << "Uncaught error: " << err.toString() << endl;
	}
	catch (std::exception & err)
	{
		Out(SYS_GEN|LOG_IMPORTANT) << "Uncaught error: " << err.what() << endl;
	}
	
	return -1;
}