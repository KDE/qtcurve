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

#include <qtcurve-utils/strs.h>
#include <assert.h>

static const char *str1 = "abcdef;;\\;;aa\\\\a;a\\bb;";
static const char *str_list1[] = {"abcdef", "", ";", "aa\\a", "abb", ""};

static const char *str2 = "abcdef,ls,\\";
static const char *str_list2[] = {"abcdef", "ls", ""};

#define _TO_STR(args...)                        \
    __TO_STR(args)
#define __TO_STR(args...)                       \
    #args

#define INT_LIST 1, 2, 3, 4, 5, 1, -2, -3, -5, 9, -10
#define FLOAT_LIST .1, 0.32, 8.3, 4.8, 4.59, 1.9, -2.10, -3.38, -8.5, 9., -10.9

static const char *int_str = _TO_STR(INT_LIST);
static const long int_list[] = {INT_LIST};

static const char *float_str = _TO_STR(FLOAT_LIST);
static const double float_list[] = {FLOAT_LIST};

typedef struct {
    const char **strs;
    int index;
} QtcStrListTest;

static bool
qtcStrListFunc(const char *str, size_t len, void *_data)
{
    QtcStrListTest *data = (QtcStrListTest*)_data;
    assert(strlen(str) == len);
    assert(strcmp(str, data->strs[data->index]) == 0);
    data->index++;
    return true;
}

int
main()
{
    QtcStrListTest test1 = {
        .strs = str_list1,
        .index = 0,
    };
    qtcStrListForEach(str1, ';', , qtcStrListFunc, &test1);
    assert(test1.index == (sizeof(str_list1) / sizeof(char*)));
    QtcStrListTest test2 = {
        .strs = str_list2,
        .index = 0,
    };
    qtcStrListForEach(str2, , , qtcStrListFunc, &test2);
    assert(test2.index == (sizeof(str_list2) / sizeof(char*)));

    size_t list1_len;
    char **list1 = qtcStrLoadStrList(str1, ';', , &list1_len, , ,);
    assert(list1_len == (sizeof(str_list1) / sizeof(char*)));
    for (unsigned i = 0;i < list1_len;i++) {
        assert(strcmp(str_list1[i], list1[i]) == 0);
        free(list1[i]);
    }
    free(list1);

    size_t list2_len;
    char **list2 = qtcStrLoadStrList(str2, , , &list2_len, , ,);
    assert(list2_len == (sizeof(str_list2) / sizeof(char*)));
    for (unsigned i = 0;i < list2_len;i++) {
        assert(strcmp(str_list2[i], list2[i]) == 0);
        free(list2[i]);
    }
    free(list2);

    size_t int_list_len;
    long *int_list_res = qtcStrLoadIntList(int_str, ',', , &int_list_len, , ,);
    assert(int_list_len == (sizeof(int_list) / sizeof(long)));
    assert(memcmp(int_list, int_list_res, sizeof(int_list)) == 0);
    free(int_list_res);

    size_t float_list_len;
    double *float_list_res = qtcStrLoadFloatList(float_str, ',', ,
                                                 &float_list_len, , ,);
    assert(float_list_len == (sizeof(float_list) / sizeof(double)));
    assert(memcmp(float_list, float_list_res, sizeof(float_list)) == 0);
    free(float_list_res);

    size_t float_list_len2 = 3;
    double static_float_list[3];
    double *float_list_res2 =
        qtcStrLoadFloatList(float_str, ',', , &float_list_len2,
                            static_float_list,
                            sizeof(static_float_list) / sizeof(double),);
    assert(float_list_len2 == (sizeof(static_float_list) / sizeof(double)));
    assert(memcmp(float_list, float_list_res2, sizeof(static_float_list)) == 0);
    return 0;
}
