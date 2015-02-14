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

#include "utils.h"

typedef struct {
    int len;
    int width;
    int height;
    int depth;
    unsigned char *data;
} QtcImage;

QTC_ALWAYS_INLINE static inline QtcImage*
qtcImageNew(int width, int height, int depth)
{
    QTC_RET_IF_FAIL(width > 0 && height > 0 && depth > 0 &&
                    depth % 8 == 0, nullptr);
    int len = width * height * depth / 8;
    QtcImage *res = qtcNewSize(QtcImage, sizeof(QtcImage) + len);
    res->len = len;
    res->width = width;
    res->height = height;
    res->depth = depth;
    res->data = ((unsigned char*)res) + sizeof(QtcImage);
    return res;
}
