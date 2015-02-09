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

static int ordered_int[1024];

static int
compare_int(const void *_left, const void *_right)
{
    const int *left = (const int*)_left;
    const int *right = (const int*)_right;
    return (*left) - (*right);
}

int
main()
{
    ordered_int[0] = 0;
    for (int i = 1;i < 1024;i++) {
        ordered_int[i] = ordered_int[i - 1] + i + (i * i * i - i * i + i) % 100;
    }
    for (int i = -100;i < ordered_int[1023] + 100;i++) {
        int *p = (int*)qtcBSearch(&i, ordered_int, 1024,
                                  sizeof(int), compare_int);
        assert(p >= ordered_int && p <= ordered_int + 1024);
        if (p == ordered_int) {
            assert(i <= ordered_int[0]);
        } else if (p == ordered_int + 1024) {
            assert(i > ordered_int[1023]);
        } else {
            assert(i <= *p && i > *(p - 1));
        }
    }
    return 0;
}
