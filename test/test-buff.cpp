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

static void
test_int_buff(unsigned size)
{
    QTC_DEF_LOCAL_BUFF(int, int_buff, 1024, size);
    assert(int_buff.l >= size);
    assert(size <= 1024 ? int_buff.p == int_buff.static_p :
           int_buff.p != int_buff.static_p);
    for (unsigned i = 0;i < size;i++) {
        int_buff.p[i] = i * i;
    }
    QTC_RESIZE_LOCAL_BUFF(int_buff, size * 2);
    assert(int_buff.l >= size * 2);
    assert(size * 2 <= 1024 ? int_buff.p == int_buff.static_p :
           int_buff.p != int_buff.static_p);
    for (unsigned i = 0;i < size * 2;i++) {
        int_buff.p[i] = i * i;
    }
    QTC_DEF_LOCAL_BUFF(int, int_buff2, 1024, size);
    QTC_FREE_LOCAL_BUFF(int_buff);
    QTC_FREE_LOCAL_BUFF(int_buff2);
}

int
main()
{
    test_int_buff(10);
    test_int_buff(100);
    test_int_buff(511);
    test_int_buff(512);
    test_int_buff(513);
    test_int_buff(1023);
    test_int_buff(1024);
    test_int_buff(1025);
    test_int_buff(10000);
    return 0;
}
