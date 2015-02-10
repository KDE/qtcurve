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

#include <chrono>
#include <vector>

static thread_local std::vector<uint64_t> qtc_tics;

namespace QtCurve {

QTC_EXPORT uint64_t
getTime()
{
    using namespace std::chrono;
    return time_point_cast<nanoseconds>(high_resolution_clock::now())
        .time_since_epoch().count();
}

QTC_EXPORT uint64_t
getElapse(uint64_t prev)
{
    return getTime() - prev;
}

QTC_EXPORT void
tic()
{
    qtc_tics.push_back(0);
    auto &back = qtc_tics.back();
    back = getTime();
}

QTC_EXPORT uint64_t
toc()
{
    uint64_t cur_time = getTime();
    if (!qtc_tics.size()) {
        return 0;
    }
    uint64_t old_time = qtc_tics.back();
    qtc_tics.pop_back();
    return cur_time - old_time;
}

}
