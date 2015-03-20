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
#ifndef _QTC_UTILS_NUMBER_H_
#define _QTC_UTILS_NUMBER_H_

#include "utils.h"

#include <cmath>
#include <cstdlib>

template <typename T1, typename T2>
static inline auto
qtcMax(const T1 &a, const T2 &b) -> decltype((a > b) ? a : b)
{
    return (a > b) ? a : b;
}
template <typename T1, typename T2>
static inline auto
qtcMin(const T1 &a, const T2 &b) -> decltype((a < b) ? a : b)
{
    return (a < b) ? a : b;
}
template <typename T>
static inline T
qtcSquare(const T &a)
{
    return a * a;
}

#define qtcBound(a, b, c) qtcMax(a, qtcMin(b, c))
#define qtcLimit(v, l) qtcBound(0, v, l)
#define qtcEqual(v1, v2) (std::abs(v1 - v2) < 0.0001)

namespace QtCurve {

template<typename T1, typename T2>
static inline auto
getPadding(T1 len, T2 align) -> decltype(len + align)
{
    if (auto left = len % align) {
        return align - left;
    }
    return 0;
}

template<typename T1, typename T2>
static inline auto
alignTo(T1 len, T2 align) -> decltype(len + align)
{
    return len + getPadding(len, align);
}

}

template<typename First>
static inline First
qtcSum(First &&first)
{
    return first;
}

template<typename First, typename... Rest>
static inline auto
qtcSum(First &&first, Rest &&...rest)
    -> decltype(first + qtcSum(std::forward<Rest>(rest)...))
{
    return first + qtcSum(std::forward<Rest>(rest)...);
}

#endif
