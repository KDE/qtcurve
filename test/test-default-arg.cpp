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

#include <qtcurve-utils/utils.h>
#include <assert.h>

static int default1_times = 0;

static int
default1()
{
    default1_times++;
    return -20;
}

static int default2_times = 0;

static int
default2()
{
    default2_times++;
    return 15;
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

#define f(i, j)                                                 \
    (f)(QTC_DEFAULT(i, default1()), QTC_DEFAULT(j, default2()))

#define ARG_MACRO (1)
#define ARG_MACRO1 ARG_MACRO
#define ARG_MACRO2 ARG_MACRO1
#define ARG_MACRO3 ARG_MACRO2
#define ARG_MACRO4 ARG_MACRO3
#define ARG_MACRO5 ARG_MACRO4
#define ARG_MACRO6 ARG_MACRO5
#define ARG_MACRO7 ARG_MACRO6
#define ARG_MACRO8 ARG_MACRO7

int
main()
{
    assert(f(,) == -5);
    assert(f(value(),) == 16);
    assert(f(, value()) == -19);
    assert(f(value(), value()) == 2);
    assert(default1_times == 2);
    assert(default2_times == 2);
    assert(value_times == 4);
    assert(f((1), (1)) == 2);
    assert(f(ARG_MACRO, ARG_MACRO) == 2);
    assert(f(ARG_MACRO1, ARG_MACRO1) == 2);
    assert(f(ARG_MACRO2, ARG_MACRO2) == 2);
    assert(f(ARG_MACRO3, ARG_MACRO3) == 2);
    assert(f(ARG_MACRO4, ARG_MACRO4) == 2);
    assert(f(ARG_MACRO5, ARG_MACRO5) == 2);
    assert(f(ARG_MACRO6, ARG_MACRO6) == 2);
    assert(f(ARG_MACRO7, ARG_MACRO7) == 2);
    assert(f(ARG_MACRO8, ARG_MACRO8) == 2);
    return 0;
}
