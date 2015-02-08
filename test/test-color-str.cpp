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

#include <qtcurve-utils/color.h>
#include <assert.h>

static void
test_short()
{
    char origin[10];
    char res[10];
    QtcColor color;
    for (int r = 0;r < 16;r++) {
        for (int g = 0;g < 16;g++) {
            for (int b = 0;b < 16;b++) {
                sprintf(origin, "#%01X%01X%01X", r, g, b);
                qtcColorFromStr(&color, origin);
                assert(color.red == r / 15.0);
                assert(color.green == g / 15.0);
                assert(color.blue == b / 15.0);
                qtcColorToStr(&color, res);
                sprintf(origin, "#%02X%02X%02X", r * 255 / 15, g * 255 / 15,
                        b * 255 / 15);
                assert(strcmp(origin, res) == 0);
            }
        }
    }
}

static void
test_long()
{
    char origin[10];
    char res[10];
    QtcColor color;
    for (int r = 0;r < 256;r++) {
        for (int g = 0;g < 256;g++) {
            for (int b = 0;b < 256;b++) {
                sprintf(origin, "#%02X%02X%02X", r, g, b);
                qtcColorFromStr(&color, origin);
                assert(color.red == r / 255.0);
                assert(color.green == g / 255.0);
                assert(color.blue == b / 255.0);
                qtcColorToStr(&color, res);
                assert(strcmp(origin, res) == 0);
            }
        }
    }
}

int
main()
{
    test_short();
    test_long();
    return 0;
}
