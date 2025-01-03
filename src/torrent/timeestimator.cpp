/*
    SPDX-FileCopyrightText: 2006 Ivan Vasić <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "timeestimator.h"
#include "torrentcontrol.h"
#include <cmath>
#include <torrent/globals.h>
#include <util/constants.h>
#include <util/log.h>
//#include "settings.h"

namespace bt
{
TimeEstimator::TimeEstimator(TorrentControl *tc)
    : m_tc(tc)
    , m_lastAvg(0)
    , m_lastETA(0)
    , m_perc(-1.0f)
{
}

TimeEstimator::~TimeEstimator()
{
}

Uint32 TimeEstimator::sample() const
{
    const TorrentStats &s = m_tc->getStats();
    if (s.completed) {
        return s.upload_rate;
    } else {
        return s.download_rate;
    }
}

Uint64 TimeEstimator::bytesLeft() const
{
    const TorrentStats &s = m_tc->getStats();
    if (s.completed) {
        if (s.max_share_ratio >= 0.01f) {
            float ratio = s.shareRatio();
            float delta = s.max_share_ratio - ratio;
            if (delta <= 0.0f)
                return 0;

            if (s.bytes_downloaded * delta < s.bytes_downloaded)
                return 0;
            else
                return s.bytes_downloaded * delta - s.bytes_uploaded;
        } else
            return 0;
    } else {
        return s.bytes_left_to_download;
    }
}

int TimeEstimator::estimate()
{
    const TorrentStats &s = m_tc->getStats();

    // in seeding mode check if we still need to seed
    if (s.completed) {
        if (bytesLeft() == 0 || s.max_share_ratio < 0.01f)
            return ALREADY_FINISHED;
    }

    // only estimate when we are running
    if (!s.running || s.paused) {
        return NEVER;
    }

    return estimateKT();
}

int TimeEstimator::estimateGASA()
{
    const TorrentStats &s = m_tc->getStats();

    if (m_tc->getRunningTimeDL() > 0 && s.bytes_downloaded > 0) {
        Uint64 d = s.bytes_downloaded;
        if (s.imported_bytes <= s.bytes_downloaded)
            d -= s.imported_bytes;
        double avg_speed = (double)d / (double)m_tc->getRunningTimeDL();
        return (Uint32)floor((double)bytesLeft() / avg_speed);
    }

    return NEVER;
}

int TimeEstimator::estimateWINX()
{
    if (m_samples.sum() > 0 && m_samples.count() > 0)
        return (Uint32)floor((double)bytesLeft() / ((double)m_samples.sum() / (double)m_samples.count()));

    return NEVER;
}

int TimeEstimator::estimateMAVG()
{
    if (m_samples.count() > 0) {
        double lavg;

        if (m_lastAvg == 0)
            lavg = (Uint32)m_samples.sum() / m_samples.count();
        else
            lavg = m_lastAvg - ((double)m_samples.first() / (double)m_samples.count()) + ((double)m_samples.last() / (double)m_samples.count());

        m_lastAvg = (Uint32)floor(lavg);

        if (lavg > 0)
            return (Uint32)floor((double)bytesLeft() / ((lavg + (m_samples.sum() / m_samples.count())) / 2));

        return NEVER;
    }

    return NEVER;
}

SampleQueue::SampleQueue()
    : m_count(0)
{
    for (int i = 0; i < SAMPLE_COUNT_MAX; ++i)
        m_samples[i] = 0;

    m_end = -1;
    m_start = 0;
}

SampleQueue::~SampleQueue()
{
}

void SampleQueue::push(Uint32 sample)
{
    if (m_count < SAMPLE_COUNT_MAX) {
        // it's not full yet
        m_samples[(++m_end) % SAMPLE_COUNT_MAX] = sample;
        m_count++;

        return;
    }

    // since it's full I'll just replace the oldest value with new one and update all variables.
    m_end = (m_end + 1) % SAMPLE_COUNT_MAX;
    m_start = (m_start + 1) % SAMPLE_COUNT_MAX;
    m_samples[m_end] = sample;
}

Uint32 SampleQueue::first()
{
    return m_samples[m_start];
}

Uint32 SampleQueue::last()
{
    return m_samples[m_end];
}

bool SampleQueue::isFull()
{
    return m_count >= SAMPLE_COUNT_MAX;
}

int SampleQueue::count()
{
    return m_count;
}

Uint32 SampleQueue::sum()
{
    Uint32 s = 0;

    for (int i = 0; i < m_count; ++i)
        s += m_samples[i];

    return s;
}

int TimeEstimator::estimateKT()
{
    const TorrentStats &s = m_tc->getStats();

    // push new sample
    m_samples.push(sample());

    if (s.completed)
        return estimateWINX();

    double perc = (double)s.bytes_downloaded / (double)s.total_bytes;

    int percentage = perc * 100;

    // calculate percentage increasement
    double delta = 1 - 1 / (perc / m_perc);

    // remember last percentage
    m_perc = perc;

    if (s.bytes_downloaded < 1024 * 100 && m_samples.last() > 0) { // < 100KB
        m_lastETA = estimateGASA();
        return m_lastETA;
    }

    if (percentage >= 99 && m_samples.last() > 0
        && bytesLeft() <= 10 * 1024 * 1024 * 1024LL) { // 1% of a very large torrent could be hundreds of MB so limit it to 10MB

        if (!m_samples.isFull()) {
            m_lastETA = estimateWINX();

            if (m_lastETA == 0)
                m_lastETA = estimateGASA();

            return m_lastETA;
        } else {
            m_lastETA = 0;

            if (delta > 0.0001)
                m_lastETA = estimateMAVG();

            if (m_lastETA == 0)
                m_lastETA = estimateGASA();
        }

        return m_lastETA;
    }

    m_lastETA = estimateMAVG();

    return m_lastETA;
}

}
