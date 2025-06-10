/*
    SPDX-FileCopyrightText: 2006 Ivan Vasić <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTTIMEESTIMATOR_H
#define BTTIMEESTIMATOR_H

#include <climits>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
class TorrentControl;

/*!
 * \brief Simple queue class for download/upload-rate samples.
 *
 * Optimized for speed and size without possibility to dynamically resize itself.
 * \author Ivan Vasic <ivasic@gmail.com>
 */
class SampleQueue
{
public:
    SampleQueue();
    ~SampleQueue();

    static constexpr auto SAMPLE_COUNT_MAX = 20;

    /*!
     * Inserts new sample into the queue. The oldest sample is overwritten.
     */
    void push(Uint32 sample);

    Uint32 first();
    Uint32 last();

    bool isFull();

    /*!
     * This function will return the number of samples in queue until it counts m_size number of elements.
     * After this point it will always return m_size since no samples are being deleted.
     */
    int count();

    /*!
     * Returns the sum of all samples.
     */
    Uint32 sum();

private:
    int m_count;

    int m_start;
    int m_end;

    Uint32 m_samples[SAMPLE_COUNT_MAX];
};

/*!
 * \brief Estimates the time remaining when downloading a torrent.
 *
 * It will use different algorithms for different download phases.
 * \author Ivan Vasic <ivasic@gmail.com>
 */
class KTORRENT_EXPORT TimeEstimator
{
public:
    static const int NEVER = INT_MAX;
    static const int ALREADY_FINISHED = 0;

    TimeEstimator(TorrentControl *tc);
    ~TimeEstimator();

    //! Returns ETA for m_tc torrent.
    int estimate();

private:
    //! global average speed algorithm
    int estimateGASA();

    //! window of X algorithm
    int estimateWINX();

    //! moving average algorithm
    int estimateMAVG();

    //! ktorrent default algorithm - combination of the following according to our tests
    int estimateKT();

    Uint32 sample() const;
    Uint64 bytesLeft() const;

    TorrentControl *m_tc;
    SampleQueue m_samples;

    Uint32 m_lastAvg;
    int m_lastETA;

    // last percentage
    double m_perc;
};

}

#endif
