/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTLOGMONITORINTERFACE_H
#define BTLOGMONITORINTERFACE_H

#include <ktorrent_export.h>

class QString;

namespace bt
{
/**
 * @author Joris Guisson
 * @brief Interface for classes who which to receive which log messages are printed
 *
 * This class is an interface for all classes which want to know,
 * what is written to the log.
 */
class KTORRENT_EXPORT LogMonitorInterface
{
public:
    LogMonitorInterface();
    virtual ~LogMonitorInterface();

    /**
     * A line was written to the log file.
     * @param line The line
     * @param filter The filter of the Log object. See Log::setFilter.
     */
    virtual void message(const QString &line, unsigned int filter) = 0;
};

}

#endif
