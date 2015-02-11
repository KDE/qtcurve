/*****************************************************************************
 *   Copyright 2015 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#include <typeinfo>
#include <iostream>

int
main()
{
    using namespace QtCurve;
    assert(oneOf(1, 1, 2, 3));
    assert(!oneOf(4, 1, 2, 3));
    assert(!noneOf(1, 1, 2, 3));
    assert(noneOf(4, 1, 2, 3));
    const char str0[] = "1234";
    const char str1[] = "1234";
    const char str2[] = "2345";
    const char str3[] = "3456";
    const char str4[] = "4567";

    assert(str0 != str1);
    static_assert(_isCharStr<char*>::value, "");
    static_assert(_isCharStr<char[]>::value, "");
    static_assert(_isCharStr<const char*>::value, "");
    static_assert(_isCharStr<const char[]>::value, "");
    static_assert(_isCharStr<decltype(str0)>::value, "");
    assert(oneOf(str0, str1, str2, str3));
    assert(!oneOf(str4, str1, str2, str3));
    assert(!noneOf(str0, str1, str2, str3));
    assert(noneOf(str4, str1, str2, str3));

    return 0;
}
