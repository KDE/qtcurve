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

#include "map.h"

#if defined(__MACH__) || defined(__APPLE__)
qtcStrMapItemCompare(void *_map, const void *_left, const void *_right)
{
    const char **left = (const char**)_left;
    const char **right = (const char**)_right;
    QtcStrMap *map = (QtcStrMap*)_map;
    return (map->case_sensitive ? strcmp(*left, *right) :
                strcasecmp(*left, *right));
}
#else
static int
qtcStrMapItemCompare(const void *_left, const void *_right, void *_map)
{
    const char **left = (const char**)_left;
    const char **right = (const char**)_right;
    QtcStrMap *map = (QtcStrMap*)_map;
    return (map->case_sensitive ? strcmp(*left, *right) :
            strcasecmp(*left, *right));
}
#endif

QTC_EXPORT void
qtcStrMapInit(QtcStrMap *map)
{
    QTC_RET_IF_FAIL(map && !map->inited && map->items && map->size && map->num);
#if defined(__MACH__) || defined(__APPLE__)
    qsort_r(map->items, map->num, map->size, map, qtcStrMapItemCompare);
#else
    qsort_r(map->items, map->num, map->size, qtcStrMapItemCompare, map);
#endif
    map->inited = true;
}

typedef struct {
    const char *key;
    bool case_sensitive;
} QtcStrMapCompKey;

static int
qtcStrMapItemCompKey(const void *_key, const void *_item)
{
    const QtcStrMapCompKey *key = (const QtcStrMapCompKey*)_key;
    const char **item = (const char**)_item;
    return (key->case_sensitive ? strcmp(key->key, *item) :
            strcasecmp(key->key, *item));
}

QTC_EXPORT void*
qtcStrMapSearch(const QtcStrMap *map, const char *key)
{
    QTC_RET_IF_FAIL(map && key, NULL);
    QtcStrMapCompKey comp_key = {
        .key = key,
        .case_sensitive = map->case_sensitive
    };
    return bsearch(&comp_key, map->items, map->num, map->size,
                   qtcStrMapItemCompKey);
}
