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

#include <qtcurve-utils/utils.h>
#include <assert.h>

static int default1_times = 0;

static int
default1(int val)
{
    default1_times++;
    return val;
}

static int default2_times = 0;

static int
default2(int val)
{
    default2_times++;
    return val;
}

static int value_times = 0;

static int
value()
{
    value_times++;
    return 1;
}

static int
f(int i, int j)
{
    return i + j;
}

int
main()
{
    int def1 = -20;
    int def2 = 15;
#define f(i, j)                                                         \
    (f)(qtcDefault(i, default1(def1)), qtcDefault(j, default2(def2)))
    assert(f(0, 0) == -5);
    assert(f(value(), 0) == 16);
    assert(f(0, value()) == -19);
    assert(f(value(), value()) == 2);
    assert(default1_times == 2);
    assert(default2_times == 2);
    assert(value_times == 4);
    return 0;
}
