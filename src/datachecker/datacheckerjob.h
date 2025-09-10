/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BT_DATACHECKERJOB_H
#define BT_DATACHECKERJOB_H

#include <torrent/job.h>
#include <util/resourcemanager.h>

namespace bt
{
class DataCheckerThread;

/*!
 * \headerfile datachecker/datacheckerjob.h
 * \brief Job which runs a DataChecker.
 */
class KTORRENT_EXPORT DataCheckerJob : public bt::Job, public Resource
{
public:
    DataCheckerJob(bool auto_import, TorrentControl *tc, bt::Uint32 from, bt::Uint32 to);
    ~DataCheckerJob() override;

    void start() override;
    void kill(bool quietly = true) override;
    [[nodiscard]] TorrentStatus torrentStatus() const override
    {
        return CHECKING_DATA;
    }

    //! Is this an automatic import
    [[nodiscard]] bool isAutoImport() const
    {
        return auto_import;
    }

    //! Was the job stopped
    [[nodiscard]] bool isStopped() const
    {
        return killed;
    }

    //! Get the first chunk the datacheck was started from
    [[nodiscard]] bt::Uint32 firstChunk() const
    {
        return from;
    }

    //! Get the last chunk of the datacheck
    [[nodiscard]] bt::Uint32 lastChunk() const
    {
        return to;
    }

private:
    void threadFinished();
    void progress(quint32 num, quint32 total);
    void status(quint32 num_failed, quint32 num_found, quint32 num_downloaded, quint32 num_not_downloaded);

    void acquired() override;

    DataCheckerThread *dcheck_thread;
    bool killed;
    bool auto_import;
    bool started;
    bt::Uint32 from;
    bt::Uint32 to;
};

}

#endif // BT_DATACHECKERJOB_H
