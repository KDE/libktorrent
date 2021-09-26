/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef NET_TRAFFICSHAPEDSOCKET_H
#define NET_TRAFFICSHAPEDSOCKET_H

#include <QMutex>
#include <net/socketdevice.h>
#include <util/constants.h>

namespace net
{
class Speed;

class SocketReader
{
public:
    SocketReader()
    {
    }
    virtual ~SocketReader()
    {
    }

    /**
     * Function which will be called whenever data has been read from the socket.
     * This data should be dealt with, otherwise it will be discarded.
     * @param buf The buffer
     * @param size The size of the buffer
     */
    virtual void onDataReady(bt::Uint8 *buf, bt::Uint32 size) = 0;
};

/**
 * Socket which supports traffic shaping
 */
class TrafficShapedSocket
{
public:
    TrafficShapedSocket(SocketDevice *sock);
    TrafficShapedSocket(int fd, int ip_version);
    TrafficShapedSocket(bool tcp, int ip_version);
    virtual ~TrafficShapedSocket();

    /// Get the SocketDevice
    SocketDevice *socketDevice()
    {
        return sock;
    }

    /// Get the SocketDevice (const vesion)
    const SocketDevice *socketDevice() const
    {
        return sock;
    }

    /// Set the reader
    void setReader(SocketReader *r)
    {
        rdr = r;
    }

    /**
     * Reads data from the socket and pass it to the SocketReader.
     * @param max_bytes_to_read Maximum number of bytes to read (0 is no limit)
     * @param now Current time stamp
     * @return The number of bytes read
     */
    virtual Uint32 read(Uint32 max_bytes_to_read, bt::TimeStamp now);

    /**
     * Writes data to the socket. Subclasses should implement the data source.
     * @param max The maximum number of bytes to send over the socket (0 = no limit)
     * @param now Current time stamp
     * @return The number of bytes written
     */
    virtual Uint32 write(Uint32 max, bt::TimeStamp now) = 0;

    /// See if the socket has something ready to write
    virtual bool bytesReadyToWrite() const = 0;

    /// Get the current download rate
    int getDownloadRate() const;

    /// Get the current download rate
    int getUploadRate() const;

    /// Update up and down speed
    void updateSpeeds(bt::TimeStamp now);

    /**
     * Set the group ID of the socket
     * @param gid THe ID (0 is default group)
     * @param upload Whether this is an upload group or a download group
     */
    void setGroupID(Uint32 gid, bool upload);

    /// Get the download group ID
    Uint32 downloadGroupID() const
    {
        return down_gid;
    }

    /// Get the upload group ID
    Uint32 uploadGroupID() const
    {
        return up_gid;
    }

protected:
    /**
     * Post process received data. Default implementation does nothing.
     * @param data The data
     * @param size The size of the data
     **/
    virtual void postProcess(bt::Uint8 *data, bt::Uint32 size);

protected:
    SocketReader *rdr;
    Speed *down_speed;
    Speed *up_speed;
    Uint32 up_gid;
    Uint32 down_gid; // group id which this torrent belongs to, group 0 means the default group
    SocketDevice *sock;
    mutable QMutex mutex;
};

}

#endif // NET_TRAFFICSHAPEDSOCKET_H
