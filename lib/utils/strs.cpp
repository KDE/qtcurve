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

#define __QTC_UTILS_STRS_INTERNAL__

#include "strs.h"
#include "number.h"

namespace QtCurve {
namespace Str {

template<bool allocated>
char*
vformat(char *buff, size_t *_size, const char *fmt, va_list ap)
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
        new_size = alignTo(new_size, 1024);
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

template QTC_EXPORT char *vformat<true>(char *buff, size_t *size,
                                        const char *fmt, va_list ap);
template QTC_EXPORT char *vformat<false>(char *buff, size_t *size,
                                         const char *fmt, va_list ap);

template<>
QTC_EXPORT double
convert<double>(const char *str, const double &def, bool *is_def)
{
    if (!str) {
        qtcAssign(is_def, true);
        return def;
    }
    str += strspn(str, " \t\b\n\f\v");
    char *end = nullptr;
    double res = strtod(str, &end);
    if (end == str) {
        qtcAssign(is_def, true);
        res = def;
    } else {
        qtcAssign(is_def, false);
    }
    return res;
}
template double convert<double>(const char*, const double&, bool *is_def);

template<>
QTC_EXPORT long
convert<long>(const char *str, const long &def, bool *is_def)
{
    if (!str) {
        qtcAssign(is_def, true);
        return def;
    }
    str += strspn(str, " \t\b\n\f\v");
    char *end = nullptr;
    long res = strtol(str, &end, 0);
    if (end == str) {
        qtcAssign(is_def, true);
        res = def;
    } else {
        qtcAssign(is_def, false);
    }
    return res;
}
template long convert<long>(const char*, const long&, bool *is_def);

template<>
QTC_EXPORT bool
convert<bool>(const char *str, const bool &def, bool *is_def)
{
    qtcAssign(is_def, false);
    if (str && *str) {
        if (def) {
            return noneOf<CaseCmp>(str, "0", "f", "false", "off");
        } else {
            return oneOf<CaseCmp>(str, "1", "t", "true", "on");
        }
    }
    return def;
}
template bool convert<bool>(const char*, const bool&, bool *is_def);

}

namespace StrList {

QTC_EXPORT void
_forEach(const char *str, char delim, char escape,
         const std::function<bool(const char*, size_t)> &func)
{
    QTC_RET_IF_FAIL(str);
    Str::Buff<1024> buff;
    if (qtcUnlikely(escape == delim)) {
        escape = '\0';
    }
    const char key[] = {delim, escape, '\0'};
    const char *p = str;
    while (true) {
        size_t len = 0;
        while (true) {
            size_t sub_len = strcspn(p, key);
            buff.resize(len + sub_len + 2);
            memcpy(buff.get() + len, p, sub_len);
            len += sub_len;
            p += sub_len;
            if (escape && *p == escape) {
                buff[len] = p[1];
                if (qtcUnlikely(!p[1])) {
                    p++;
                    break;
                }
                len++;
                p += 2;
            } else {
                buff[len] = '\0';
                break;
            }
        }
        if (!func(buff.get(), len) || !*p) {
            break;
        }
        p++;
    }
}
}

}

QTC_EXPORT void*
qtcStrLoadList(const char *str, char delim, char escape, size_t size,
               size_t *_nele, void *buff, size_t max_len,
               QtcListEleLoader loader, void *data)
{
    QTC_RET_IF_FAIL(_nele && size && loader && str, nullptr);
    size_t nele = *_nele;
    size_t offset = 0;
    if (!(buff && nele)) {
        nele = 16;
        buff = malloc(16 * size);
    }
    QtCurve::StrList::forEach(
        str, delim, escape, [&] (const char *str, size_t len) {
            if (nele <= offset) {
                nele += 8;
                buff = (char*)realloc(buff, nele * size);
            }
            if (loader((char*)buff + offset * size, str, len, data)) {
                offset++;
                if (max_len && offset >= max_len) {
                    return false;
                }
            }
            return true;
        });
    *_nele = offset;
    if (!*_nele) {
        free(buff);
        return nullptr;
    }
    return buff;
}

static bool
qtcStrListStrLoader(void *ele, const char *str, size_t len, void *data)
{
    const char *def = (const char*)data;
    if (def && !str[0]) {
        *(char**)ele = strdup(def);
    } else {
        *(char**)ele = QtCurve::Str::dup(nullptr, str, len);
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
    *(long*)ele = QtCurve::Str::convert(str, (long)(intptr_t)data);
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
    *(double*)ele = QtCurve::Str::convert(str, *(double*)data);
    return true;
}

QTC_EXPORT double*
qtcStrLoadFloatList(const char *str, char delim, char escape, size_t *nele,
                    double *buff, size_t max_len, double def)
{
    return (double*)qtcStrLoadList(str, delim, escape, sizeof(double), nele,
                                   buff, max_len, qtcStrListFloatLoader, &def);
}
