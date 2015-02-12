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

#include "strs.h"
#include "number.h"

QTC_EXPORT char*
_qtcSPrintf(char *buff, size_t *size, bool allocated, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *res = _qtcSPrintfV(buff, size, allocated, fmt, ap);
    va_end(ap);
    return res;
}

QTC_EXPORT char*
_qtcSPrintfV(char *buff, size_t *_size, bool allocated,
             const char *fmt, va_list ap)
{
    if (!buff || !_size || !*_size) {
        char *res = nullptr;
        vasprintf(&res, fmt, ap);
        return res;
    }
    va_list _ap;
    va_copy(_ap, ap);
    size_t size = *_size;
    size_t new_size = vsnprintf(buff, size, fmt, ap) + 1;
    if (new_size > size) {
        new_size = qtcAlignTo(new_size, 1024);
        if (allocated) {
            buff = (char*)realloc(buff, new_size);
        } else {
            buff = (char*)malloc(new_size);
        }
        *_size = new_size;
        new_size = vsnprintf(buff, new_size, fmt, _ap);
    }
    return buff;
}

QTC_EXPORT void
qtcStrListForEach(const char *str, char delim, char escape,
                  QtcListForEachFunc func, void *data)
{
    QTC_RET_IF_FAIL(str && func);
    QtCurve::StrBuff<1024> str_buff;
    if (qtcUnlikely(escape == delim)) {
        escape = '\0';
    }
    const char key[] = {delim, escape, '\0'};
    const char *p = str;
    while (true) {
        size_t len = 0;
        while (true) {
            size_t sub_len = strcspn(p, key);
            str_buff.resize(len + sub_len + 2);
            memcpy(str_buff.get() + len, p, sub_len);
            len += sub_len;
            p += sub_len;
            if (escape && *p == escape) {
                str_buff[len] = p[1];
                if (qtcUnlikely(!p[1])) {
                    p++;
                    break;
                }
                len++;
                p += 2;
            } else {
                str_buff[len] = '\0';
                break;
            }
        }
        if (!func(str_buff.get(), len, data) || !*p) {
            break;
        }
        p++;
    }
}

typedef struct {
    size_t size;
    size_t nele;
    void *buff;
    size_t max_len;
    QtcListEleLoader loader;
    void *data;
    size_t offset;
} QtcStrLoadListData;

static bool
qtcStrListLoader(const char *str, size_t len, void *_data)
{
    QtcStrLoadListData *data = (QtcStrLoadListData*)_data;
    if (data->nele <= data->offset) {
        data->nele += 8;
        data->buff = (char*)realloc(data->buff, data->nele * data->size);
    }
    if (data->loader((char*)data->buff + data->offset * data->size,
                     str, len, data->data)) {
        data->offset++;
        if (data->max_len && data->offset >= data->max_len) {
            return false;
        }
    }
    return true;
}

QTC_EXPORT void*
qtcStrLoadList(const char *str, char delim, char escape, size_t size,
               size_t *_nele, void *buff, size_t max_len,
               QtcListEleLoader loader, void *data)
{
    QTC_RET_IF_FAIL(_nele && size && loader && str, nullptr);
    QtcStrLoadListData loader_data = {
        .size = size,
        .nele = *_nele,
        .buff = buff,
        .max_len = max_len,
        .loader = loader,
        .data = data,
        .offset = 0,
    };
    if (!(loader_data.buff && loader_data.nele)) {
        loader_data.nele = 16;
        loader_data.buff = malloc(16 * size);
    }
    qtcStrListForEach(str, delim, escape, qtcStrListLoader, &loader_data);
    *_nele = loader_data.offset;
    if (!*_nele) {
        free(loader_data.buff);
        return nullptr;
    }
    return loader_data.buff;
}

static bool
qtcStrListStrLoader(void *ele, const char *str, size_t len, void *data)
{
    const char *def = (const char*)data;
    if (def && !str[0]) {
        *(char**)ele = strdup(def);
    } else {
        *(char**)ele = qtcSetStr(nullptr, str, len);
    }
    return true;
}

QTC_EXPORT char**
qtcStrLoadStrList(const char *str, char delim, char escape, size_t *nele,
                  char **buff, size_t max_len, const char *def)
{
    return (char**)qtcStrLoadList(str, delim, escape, sizeof(char*), nele, buff,
                                  max_len, qtcStrListStrLoader, (void*)def);
}

static bool
qtcStrListIntLoader(void *ele, const char *str, size_t, void *data)
{
    long def = (long)(intptr_t)data;
    str += strspn(str, " \t\b\n\f\v");
    char *end = nullptr;
    long res = strtol(str, &end, 0);
    if (end == str) {
        res = def;
    }
    *(long*)ele = res;
    return true;
}

QTC_EXPORT long*
qtcStrLoadIntList(const char *str, char delim, char escape, size_t *nele,
                  long *buff, size_t max_len, long def)
{
    return (long*)qtcStrLoadList(str, delim, escape, sizeof(long), nele, buff,
                                 max_len, qtcStrListIntLoader,
                                 (void*)(intptr_t)def);
}

static bool
qtcStrListFloatLoader(void *ele, const char *str, size_t, void *data)
{
    double def = *(double*)data;
    str += strspn(str, " \t\b\n\f\v");
    char *end = nullptr;
    double res = strtod(str, &end);
    if (end == str) {
        res = def;
    }
    *(double*)ele = res;
    return true;
}

QTC_EXPORT double*
qtcStrLoadFloatList(const char *str, char delim, char escape, size_t *nele,
                    double *buff, size_t max_len, double def)
{
    return (double*)qtcStrLoadList(str, delim, escape, sizeof(double), nele,
                                   buff, max_len, qtcStrListFloatLoader, &def);
}
