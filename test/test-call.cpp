/*****************************************************************************
 *   Copyright 2014 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
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

static int (*func_int)(int a) = nullptr;
static void (*func_void)() = nullptr;

static int arg_int_times = 0;

static int
arg_int()
{
    arg_int_times++;
    return 10;
}

static int
real_func_int(int a)
{
    return a;
}

static int real_func_void_times = 0;

static void
real_func_void()
{
    real_func_void_times++;
}

#ifdef __cplusplus
// Functions with structs as return type only works for c++ (for now).
typedef struct {
    int i;
    int j;
} TestStruct;

static TestStruct (*func_struct)() = nullptr;
#endif

int
main()
{
    qtcCall(func_void);
    assert(qtcCall(func_int, arg_int()) == 0);
    assert(arg_int_times == 0);
    func_int = real_func_int;
    assert(qtcCall(func_int, arg_int()) == 10);
    assert(arg_int_times == 1);
    func_void = real_func_void;
    qtcCall(func_void);
    assert(real_func_void_times == 1);
#ifdef __cplusplus
    qtcCall(func_struct);
#endif
    return 0;
}
