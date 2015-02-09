/*****************************************************************************
 *   Copyright 2013 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef _QTC_UTILS_MAP_H_
#define _QTC_UTILS_MAP_H_

#include "utils.h"

#include <vector>
#include <algorithm>
#include <initializer_list>

namespace QtCurve {

// C++14 integer sequence
template<int...>
struct seq {
};

template<int N, int... S>
struct gens : gens<N - 1, N - 1, S...> {
};

template<int ...S>
struct gens<0, S...> {
    typedef seq<S...> type;
};

template<int N>
using gens_t = typename gens<N>::type;

template<typename Val=int, bool case_sens=true>
class StrMap : std::vector<std::pair<const char*, Val> > {
    typedef std::pair<const char*, Val> pair_type;
    static int
    strcmp_func(const char *a, const char *b)
    {
        if (case_sens) {
            return strcmp(a, b);
        } else {
            return strcasecmp(a, b);
        }
    }
    template<typename... Ts, int ...S>
    StrMap(seq<S...>, Ts&&... ts)
        : StrMap{{ts, Val(S)}...}
    {
    }
public:
    StrMap(std::initializer_list<pair_type> &&ts)
        : std::vector<pair_type>(std::move(ts))
    {
        std::sort(this->begin(), this->end(),
                  [] (const pair_type &a, const pair_type &b) {
                      return strcmp_func(a.first, b.first) < 0;
                  });
    }
    template<typename... Ts>
    StrMap(Ts&&... ts)
        : StrMap(gens_t<sizeof...(Ts)>(), std::forward<Ts>(ts)...)
    {
    }
    Val
    search(const char *key, Val def=Val(-1), bool *is_def=nullptr) const
    {
        QTC_RET_IF_FAIL(key, def);
        auto lower_it = std::lower_bound(
            this->begin(), this->end(), key, [] (const pair_type &a,
                                                 const char *key) {
                return strcmp_func(a.first, key) < 0;
            });
        if (lower_it == this->end() ||
            strcmp_func(lower_it->first, key) != 0) {
            qtcAssign(is_def, true);
            return def;
        }
        qtcAssign(is_def, false);
        return lower_it->second;
    }
};

}

#endif
