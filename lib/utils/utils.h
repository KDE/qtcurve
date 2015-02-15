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

#ifndef _QTC_UTILS_UTILS_H_
#define _QTC_UTILS_UTILS_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>

#include "macros.h"

#include <memory>
#include <utility>
#include <type_traits>
#include <initializer_list>

/**
 * \file utils.h
 * \author Yichao Yu <yyc1992@gmail.com>
 * \brief Some generic functions and macros.
 */

/**
 * Generic call back function.
 */
typedef void (*QtcCallback)(void*);

/**
 * Allocate memory and initialize it to zero.
 * \param size size of the memory
 */
QTC_ALWAYS_INLINE static inline void*
qtcAlloc0(size_t size)
{
    void *p = malloc(size);
    memset(p, 0, size);
    return p;
}

/**
 * Allocate memory of size \param size for a \param type.
 * The memory is initialized to zero.
 * \param type type pointed to by the pointer
 * \param size size of the memory
 * \sa qtcAlloc0
 */
#define qtcNewSize(type, size) ((type*)qtcAlloc0(size))

/**
 * Allocate memory for a \param type or an array of \param type.
 * The memory is initialized to zero.
 * \param type type pointed to by the pointer
 * \param n (optional) elements in the array (default: 1)
 * \sa qtcNewSize
 */
#define qtcNew(type, n...)                              \
    qtcNewSize(type, sizeof(type) * (QTC_DEFAULT(n, 1)))

namespace QtCurve {

template<typename T>
using remove_cvr_t = typename std::remove_cv<
    typename std::remove_reference<T>::type>::type;

template<typename T>
struct _isChar : std::is_same<remove_cvr_t<T>, char> {};

template<typename T>
struct _isCharPtr : std::integral_constant<
    bool,
    std::is_pointer<remove_cvr_t<T> >::value &&
    _isChar<typename std::remove_pointer<remove_cvr_t<T> >::type>::value> {};

template<typename T>
struct _isCharAry : std::integral_constant<
    bool,
    std::is_array<remove_cvr_t<T> >::value &&
    _isChar<typename std::remove_extent<remove_cvr_t<T> >::type>::value> {};

template<typename T>
struct _isCharStr : std::integral_constant<
    bool, _isCharPtr<T>::value || _isCharAry<T>::value> {};

template<typename T1, typename T2, class=void>
struct _oneOfCmp {
    inline bool operator()(T1 &&v1, T2 &&v2)
    {
        return std::forward<T1>(v1) == std::forward<T2>(v2);
    }
};

template<typename T1, typename T2>
struct _oneOfCmp<T1, T2, typename std::enable_if<_isCharStr<T1>::value &&
                                      _isCharStr<T2>::value>::type> {
    inline bool operator()(T1 &&v1, T2 &&v2)
    {
        return strcmp(v1, v2) == 0;
    }
};

template<typename...>
struct CaseCmp {
    inline bool operator()(const char *str1, const char *str2)
    {
        return strcasecmp(str1, str2) == 0;
    }
};

template<template<typename...> class _Cmp=_oneOfCmp,
         typename T1, typename T2>
static inline bool
oneOf(T1 &&v1, T2 &&v2)
{
    return _Cmp<T1, T2>()(std::forward<T1>(v1), std::forward<T2>(v2));
}

template<template<typename...> class _Cmp=_oneOfCmp,
         typename T, typename First, typename... Rest>
static inline bool
oneOf(T &&value, First &&first, Rest&&... rest)
{
    return (oneOf<_Cmp>(std::forward<T>(value), std::forward<First>(first)) ||
            oneOf<_Cmp>(std::forward<T>(value), std::forward<Rest>(rest)...));
}

template<template<typename...> class _Cmp=_oneOfCmp,
         typename... Args>
static inline bool
noneOf(Args&&... args)
{
    return !oneOf<_Cmp>(std::forward<Args>(args)...);
}

/**
 * Turn a variable into a const reference. This is useful for range-based for
 * loop where a non-const variable may cause unnecessary copy.
 */
template <class T>
static inline const T&
const_(const T &t)
{
    return t;
}

struct CDeleter {
    template<typename T>
    void operator()(T *p)
    {
        free((void*)p);
    }
};

template<typename T>
using uniqueCPtr = std::unique_ptr<T, CDeleter>;

template<typename T, size_t N>
class LocalBuff {
    LocalBuff(const LocalBuff&) = delete;
public:
    LocalBuff(size_t size=N, const T *ary=nullptr)
        : m_ptr(size > N ? qtcNew(T, size) : m_static_buf),
          m_size(size),
          m_static_buf{}
    {
        if (ary) {
            memcpy(m_ptr, ary, sizeof(T) * size);
        }
    }
    bool
    is_static() const
    {
        return m_ptr == m_static_buf;
    }
    void
    resize(size_t size)
    {
        if (is_static()) {
            if (size > N) {
                m_ptr = qtcNew(T, size);
                memcpy(m_ptr, m_static_buf, sizeof(T) * m_size);
            }
        } else {
            m_ptr = (T*)realloc(m_ptr, sizeof(T) * size);
        }
        m_size = size;
    }
    T*
    get() const
    {
        return m_ptr;
    }
    T&
    operator[](size_t i) const
    {
        return get()[i];
    }
    size_t
    size() const
    {
        return m_size;
    }
    ~LocalBuff()
    {
        if (!is_static()) {
            free(m_ptr);
        }
    }
protected:
    T *m_ptr;
    size_t m_size;
private:
    T m_static_buf[N];
};
}

template<typename... Args>
static inline bool
qtcOneOf(Args&&... args)
{
    return QtCurve::oneOf(std::forward<Args>(args)...);
}
const char *qtcGetProgName();
const char *qtcVersion();

template<typename T>
using qtcPtrType = QtCurve::remove_cvr_t<typename std::remove_pointer<T>::type>;

#define qtcMemPtr(ptr, name) &qtcPtrType<decltype(ptr)>::name

// Use lambda for lazy evaluation of \param def
#define qtcDefault(val, def)                    \
    (([&]() {                                   \
            auto __val = (val);                 \
            return __val ? __val : (def);       \
        })())
// Use lambda for lazy evaluation of \param args
// C++ allows returning void expression! =) See the quote of the standard
// (here)[http://gcc.gnu.org/ml/gcc/2006-10/msg00697.html]
// The current c++ implementation of this macro does not support functions
// with types that do not have accessible default constructor (including
// references) as return type.
#define qtcCall(func, args...)                                          \
    (([&]() {                                                           \
            auto __func = (func);                                       \
            return __func ? __func(args) : decltype(__func(args))();    \
        })())
#define qtcAssign(addr, exp) do {               \
        auto __addr = (addr);                   \
        if (__addr) {                           \
            *__addr = (exp);                    \
        }                                       \
    } while(0)

// Returning a void expression is valid c++, see above
// Or https://gcc.gnu.org/ml/gcc/2006-10/msg00697.html
#define QTC_RET_IF_FAIL(exp, val...) do {       \
        if (!qtcLikely(exp)) {                  \
            return (QTC_DEFAULT(val, (void)0)); \
        }                                       \
    } while (0)

#endif
