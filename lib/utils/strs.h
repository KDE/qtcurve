/*****************************************************************************
 *   Copyright 2012 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef _QTC_UTILS_STRS_H_
#define _QTC_UTILS_STRS_H_

#include "utils.h"
#include <stdarg.h>

#include <array>
#include <numeric>

__attribute__((format(printf, 4, 5)))
char *_qtcSPrintf(char *buff, size_t *size, bool allocated,
                  const char *fmt, ...);
__attribute__((format(printf, 4, 0)))
char *_qtcSPrintfV(char *buff, size_t *size, bool allocated,
                   const char *fmt, va_list ap);

namespace QtCurve {

template <typename... ArgTypes>
QTC_ALWAYS_INLINE static inline char*
fillStrs(char *buff, ArgTypes&&...strs)
{
    const std::array<const char*, sizeof...(strs)> strs_l{{strs...}};
    const std::array<size_t, sizeof...(strs)> str_lens{{strlen(strs)...}};
    const size_t total_len = std::accumulate(str_lens.begin(),
                                             str_lens.end(), 0);
    char *res = (buff ? (char*)realloc(buff, total_len + 1) :
                 (char*)malloc(total_len + 1));
    char *p = res;
    for (size_t i = 0;i < sizeof...(strs);i++) {
        memcpy(p, strs_l[i], str_lens[i]);
        p += str_lens[i];
    }
    res[total_len] = 0;
    return res;
}

template <typename... ArgTypes>
QTC_ALWAYS_INLINE static inline char*
catStrs(ArgTypes&&... strs)
{
    return fillStrs(nullptr, std::forward<ArgTypes>(strs)...);
}

template<size_t N>
class StrBuff : public LocalBuff<char, N> {
public:
    using LocalBuff<char, N>::LocalBuff;
    __attribute__((format(printf, 2, 3)))
    char*
    printf(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        if (this->is_static()) {
            size_t new_size = N;
            char *res = _qtcSPrintfV(this->m_ptr, &new_size, false, fmt, ap);
            if (res != this->m_ptr) {
                this->m_ptr = res;
                this->m_size = new_size;
            }
        } else {
            this->m_ptr = _qtcSPrintfV(this->m_ptr, &this->m_size,
                                       true, fmt, ap);
        }
        va_end(ap);
        return this->m_ptr;
    }
    template <typename... ArgTypes>
    char*
    cat_strs(ArgTypes&&...strs)
    {
        const std::array<const char*, sizeof...(strs)> strs_l{{strs...}};
        const std::array<size_t, sizeof...(strs)> str_lens{{strlen(strs)...}};
        const size_t total_len = std::accumulate(str_lens.begin(),
                                                 str_lens.end(), 0);
        this->resize(total_len);
        char *p = this->m_ptr;
        for (size_t i = 0;i < sizeof...(strs);i++) {
            memcpy(p, strs_l[i], str_lens[i]);
            p += str_lens[i];
        }
        this->m_ptr[total_len] = 0;
        return this->m_ptr;
    }
};

}

QTC_ALWAYS_INLINE static inline char*
qtcSetStr(char *dest, const char *src, size_t len)
{
    dest = (char*)realloc(dest, len + 1);
    memcpy(dest, src, len);
    dest[len] = '\0';
    return dest;
}
QTC_ALWAYS_INLINE static inline char*
qtcSetStr(char *dest, const char *src)
{
    return qtcSetStr(dest, src, strlen(src));
}

#define qtcSPrintfV(buff, size, fmt, ap)        \
    _qtcSPrintfV(buff, size, true, fmt, ap)
#define qtcSPrintf(buff, size, fmt, ap, args...)        \
    _qtcSPrintf(buff, size, true, fmt, ap, ##args)

__attribute__((format(printf, 3, 0)))
QTC_ALWAYS_INLINE static inline char*
qtcASNPrintfV(char *buff, size_t size, const char *fmt, va_list ap)
{
    return qtcSPrintfV(buff, &size, fmt, ap);
}

__attribute__((format(printf, 3, 4)))
QTC_ALWAYS_INLINE static inline char*
qtcASNPrintf(char *buff, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *res = qtcASNPrintfV(buff, size, fmt, ap);
    va_end(ap);
    return res;
}

#define qtcASPrintfV(fmt, ap)                   \
    qtcASNPrintfV(nullptr, 0, fmt, ap)
#define qtcASPrintf(fmt, args...)               \
    qtcASNPrintf(nullptr, 0, fmt, ##args)

typedef bool (*QtcListForEachFunc)(const char *str, size_t len, void *data);
void qtcStrListForEach(const char *str, char delim, char escape,
                       QtcListForEachFunc func, void *data);
#define qtcStrListForEach(str, delim, escape, func, data)               \
    qtcStrListForEach(str, QTC_DEFAULT(delim, ','),                     \
                      QTC_DEFAULT(escape, '\\'), func, QTC_DEFAULT(data, nullptr))

typedef bool (*QtcListEleLoader)(void *ele, const char *str,
                                 size_t len, void *data);

void *qtcStrLoadList(const char *str, char delim, char escape,
                     size_t size, size_t *nele, void *buff, size_t max_len,
                     QtcListEleLoader loader, void *data=nullptr);
#define qtcStrLoadList(str, delim, escape, size, nele,                  \
                       buff, max_len, loader, data...)                  \
    qtcStrLoadList(str, QTC_DEFAULT(delim, ','), QTC_DEFAULT(escape, '\\'), \
                   size, nele, QTC_DEFAULT(buff, nullptr),              \
                   QTC_DEFAULT(max_len, 0), loader, ##data)

char **qtcStrLoadStrList(const char *str, char delim, char escape, size_t *nele,
                         char **buff, size_t max_len, const char *def);
#define qtcStrLoadStrList(str, delim, escape, nele, buff, max_len, def) \
    qtcStrLoadStrList(str, QTC_DEFAULT(delim, ','),                     \
                      QTC_DEFAULT(escape, '\\'), nele,                  \
                      QTC_DEFAULT(buff, nullptr), QTC_DEFAULT(max_len, 0), \
                      QTC_DEFAULT(def, nullptr))

long *qtcStrLoadIntList(const char *str, char delim, char escape,
                        size_t *nele, long *buff=nullptr,
                        size_t max_len=0, long def=0);
#define qtcStrLoadIntList(str, delim, escape, nele, extra...)   \
    qtcStrLoadIntList(str, QTC_DEFAULT(delim, ','),             \
                      QTC_DEFAULT(escape, '\\'), nele, ##extra)

double *qtcStrLoadFloatList(const char *str, char delim, char escape,
                            size_t *nele, double *buff=nullptr,
                            size_t max_len=0, double def=0);
#define qtcStrLoadFloatList(str, delim, escape, nele, extra...)         \
    qtcStrLoadFloatList(str, QTC_DEFAULT(delim, ','),                   \
                        QTC_DEFAULT(escape, '\\'), nele, ##extra)

QTC_ALWAYS_INLINE static inline bool
qtcStrToBool(const char *str, bool def)
{
    if (str && *str) {
        if (def) {
            return !(strcasecmp(str, "0") == 0 || strcasecmp(str, "f") == 0 ||
                     strcasecmp(str, "false") == 0 ||
                     strcasecmp(str, "off") == 0);
        } else {
            return (strcasecmp(str, "1") == 0 || strcasecmp(str, "t") == 0 ||
                    strcasecmp(str, "true") == 0 || strcasecmp(str, "on") == 0);
        }
    }
    return def;
}

QTC_ALWAYS_INLINE static inline long
qtcStrToInt(const char *str, long def, bool *is_def=nullptr)
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

QTC_ALWAYS_INLINE static inline double
qtcStrToFloat(const char *str, double def, bool *is_def=nullptr)
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

QTC_ALWAYS_INLINE static inline bool
qtcStrEndsWith(const char *str, const char *tail)
{
    size_t len1 = strlen(str);
    size_t len2 = strlen(tail);
    if (len2 > len1) {
        return false;
    }
    return memcmp(str + len1 - len2, tail, len2) == 0;
}

QTC_ALWAYS_INLINE static inline bool
qtcStrStartsWith(const char *str, const char *head)
{
    return strncmp(str, head, strlen(head)) == 0;
}

#endif
