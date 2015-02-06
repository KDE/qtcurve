/*****************************************************************************
 *   Copyright 2013 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#include "timer.h"
#include "list.h"
#include "number.h"
#include <time.h>
#include <vector>

#if defined(__APPLE__) || defined(__MACH__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/mach_init.h>
#include <sys/sysctl.h>

static mach_timebase_info_data_t sTimebaseInfo;
static double calibrator = 0;

__attribute__((constructor)) static void
init_HRTime()
{
    if (!calibrator) {
        mach_timebase_info(&sTimebaseInfo);
        /* go from absolute time units to nanoseconds: */
        calibrator = (double)sTimebaseInfo.numer / (double)sTimebaseInfo.denom;
        /* fprintf(stderr, "init_HRTime(): calibrator=%g\n", calibrator); */
    }
}

QTC_EXPORT uint64_t
qtcGetTime()
{
    return (uint64_t)mach_absolute_time() * calibrator;
}

#else

#ifdef CLOCK_THREAD_CPUTIME_ID
#  define CLOCK_ID CLOCK_THREAD_CPUTIME_ID
#else
#  define CLOCK_ID CLOCK_MONOTONIC
#endif

QTC_EXPORT uint64_t
qtcGetTime()
{
    struct timespec time_spec;
    clock_gettime(CLOCK_ID, &time_spec);
    return ((uint64_t)time_spec.tv_sec) * 1000000000ull + time_spec.tv_nsec;
}
#endif // __APPLE__

QTC_EXPORT uint64_t
qtcGetElapse(uint64_t prev)
{
    return qtcGetTime() - prev;
}

static thread_local std::vector<uint64_t> qtc_tics;

QTC_EXPORT void
qtcTic()
{
    qtc_tics.push_back(0);
    auto &back = qtc_tics.back();
    back = qtcGetTime();
}

QTC_EXPORT uint64_t
qtcToc()
{
    uint64_t cur_time = qtcGetTime();
    if (!qtc_tics.size()) {
        return 0;
    }
    uint64_t old_time = qtc_tics.back();
    qtc_tics.pop_back();
    return cur_time - old_time;
}
