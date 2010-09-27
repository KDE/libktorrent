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

#ifndef BT_TORRENTFILESTREAM_H
#define BT_TORRENTFILESTREAM_H

#include <QIODevice>
#include <ktorrent_export.h>
#include <util/constants.h>


namespace bt 
{
	class ChunkManager;
	class TorrentControl;
	class TorrentInterface;
	class ChunkSelectorDataProvider;
	
	
	/**
		QIODevice which streams a file of a torrent or the whole torrent (for single file torrents)
		This object should not be manually constructed.
	*/
	class KTORRENT_EXPORT TorrentFileStream : public QIODevice
	{
		Q_OBJECT
	public:
		TorrentFileStream(TorrentControl* tc,ChunkManager* cman,QObject* parent);
		TorrentFileStream(TorrentControl* tc,Uint32 file_index,ChunkManager* cman,QObject* parent);
		virtual ~TorrentFileStream();
		
		/// Open the device (only readonly access will be allowed)
		virtual bool open(QIODevice::OpenMode mode);
		
		/// Close the device
		virtual void close();
		
		/// Get the current stream position
		virtual qint64 pos() const;
		
		/// Get the total size
		virtual qint64 size() const;
		
		/// Seek, will fail if attempting to seek to a point which is not downloaded yet
		virtual bool seek(qint64 pos);
		
		/// Are we at the end of the file
		virtual bool atEnd() const;
		
		/// Reset the stream
		virtual bool reset();
		
		/// How many bytes are there available
		virtual qint64 bytesAvailable() const;
		
		/// The stream is not sequential
		virtual bool isSequential() const {return false;}
		
		/// Get the path of the file
		QString path() const;
		
	protected:
		virtual qint64 writeData(const char* data, qint64 len);
		virtual qint64 readData(char* data, qint64 maxlen);
		void emitReadChannelFinished();
		
	private slots:
		void chunkDownloaded(TorrentInterface* tc, Uint32 chunk);
		
	private:
		class Private;
		Private* d;
	};

}

#endif // BT_TORRENTFILESTREAM_H
