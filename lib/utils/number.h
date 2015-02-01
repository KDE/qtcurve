/*****************************************************************************
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifdef __cplusplus

template <typename T>
QTC_ALWAYS_INLINE static inline const T&
qtcMax(const T &a, const T &b)
{
    return (a > b) ? a : b;
}
template <typename T>
QTC_ALWAYS_INLINE static inline const T&
qtcMin(const T &a, const T &b)
{
    return (a < b) ? a : b;
}
template <typename T1, typename T2>
QTC_ALWAYS_INLINE static inline auto
qtcMax(const T1 &a, const T2 &b) -> decltype((a > b) ? a : b)
{
    return (a > b) ? a : b;
}
template <typename T1, typename T2>
QTC_ALWAYS_INLINE static inline auto
qtcMin(const T1 &a, const T2 &b) -> decltype((a < b) ? a : b)
{
    return (a < b) ? a : b;
}
template <typename T>
QTC_ALWAYS_INLINE static inline T
qtcAbs(const T &a)
{
    return (a > 0) ? a : -a;
}
template <typename T>
QTC_ALWAYS_INLINE static inline T
qtcSquare(const T &a)
{
    return a * a;
}
#else
#define qtcMax(a, b)                            \
    ({                                          \
        typeof(a) _a = (a);                     \
        typeof(b) _b = (b);                     \
        (_a > _b) ? _a : _b;                    \
    })
#define qtcMin(a, b)                            \
    ({                                          \
        typeof(a) _a = (a);                     \
        typeof(b) _b = (b);                     \
        (_a < _b) ? _a : _b;                    \
    })
#define qtcAbs(a)                               \
    ({                                          \
        typeof(a) _a = (a);                     \
        (_a > 0) ? _a : -_a;                    \
    })
#define qtcSquare(a)                            \
    ({                                          \
        typeof(a) _a = (a);                     \
        _a * _a;                                \
    })
#endif
#define qtcBound(a, b, c) qtcMax(a, qtcMin(b, c))
#define qtcLimit(v, l) qtcBound(0, v, l)
#define qtcEqual(v1, v2) (qtcAbs(v1 - v2) < 0.0001)

QTC_ALWAYS_INLINE static inline uintptr_t
qtcGetPadding(uintptr_t len, uintptr_t align)
{
    uintptr_t left;
    if ((left = len % align)) {
        return align - left;
    }
    return 0;
}

QTC_ALWAYS_INLINE static inline uintptr_t
qtcAlignTo(uintptr_t len, uintptr_t align)
{
    return len + qtcGetPadding(len, align);
}

#endif
