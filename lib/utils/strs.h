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
#include <functional>
#include <string>

namespace QtCurve {

class uniqueStr : public uniqueCPtr<char> {
public:
    // TODO add back when when we drop gcc 4.7 support.
    // using uniqueCPtr<char>::uniqueCPtr;
    uniqueStr(char *p)
        : uniqueCPtr<char>(p)
    {
    }
    template<typename T,
             typename std::enable_if<
                 std::is_convertible<decltype(std::declval<T>()()),
                                     char*>::value>::type* = nullptr>
    uniqueStr(T &&f)
        : uniqueStr((char*)f())
    {
    }
};

namespace Str {

QTC_ALWAYS_INLINE static inline bool
startsWith(const char *str, const char *head)
{
    return strncmp(str, head, strlen(head)) == 0;
}

QTC_ALWAYS_INLINE static inline bool
endsWith(const char *str, const char *tail)
{
    size_t len1 = strlen(str);
    size_t len2 = strlen(tail);
    if (len2 > len1) {
        return false;
    }
    return memcmp(str + len1 - len2, tail, len2) == 0;
}

QTC_ALWAYS_INLINE static inline char*
dup(char *dest, const char *src, size_t len)
{
    dest = (char*)realloc(dest, len + 1);
    memcpy(dest, src, len);
    dest[len] = '\0';
    return dest;
}
QTC_ALWAYS_INLINE static inline char*
dup(char *dest, const char *src)
{
    return dup(dest, src, strlen(src));
}
QTC_ALWAYS_INLINE static inline char*
dup(const char *src)
{
    return strdup(src);
}

template<bool allocated=true> __attribute__((format(printf, 3, 0)))
char *vformat(char *buff, size_t *size, const char *fmt, va_list ap);

#ifndef __QTC_UTILS_STRS_INTERNAL__
extern template __attribute__((format(printf, 3, 0)))
char *vformat<true>(char *buff, size_t *size, const char *fmt, va_list ap);
extern template __attribute__((format(printf, 3, 0)))
char *vformat<false>(char *buff, size_t *size, const char *fmt, va_list ap);
#endif

template<bool allocated=true>
__attribute__((format(printf, 3, 4)))
static inline char*
format(char *buff, size_t *size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *res = vformat<allocated>(buff, size, fmt, ap);
    va_end(ap);
    return res;
}

template <typename... ArgTypes>
QTC_ALWAYS_INLINE static inline char*
fill(char *buff, ArgTypes&&...strs)
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
cat(ArgTypes&&... strs)
{
    return fill(nullptr, std::forward<ArgTypes>(strs)...);
}

template<size_t N>
class Buff : public LocalBuff<char, N> {
public:
    Buff(size_t size=N, const char *ary=nullptr)
        : LocalBuff<char, N>(size, ary)
    {}
    Buff(const char *str)
        : Buff(str ? strlen(str) + 1 : 0, str)
    {}
    operator char*() const
    {
        return this->get();
    }
    __attribute__((format(printf, 2, 3)))
    char*
    printf(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        if (this->is_static()) {
            size_t new_size = N;
            char *res = vformat<false>(this->m_ptr, &new_size, fmt, ap);
            if (res != this->m_ptr) {
                this->m_ptr = res;
                this->m_size = new_size;
            }
        } else {
            this->m_ptr = vformat(this->m_ptr, &this->m_size, fmt, ap);
        }
        va_end(ap);
        return this->m_ptr;
    }
    template <typename... ArgTypes>
    inline char*
    cat(ArgTypes&&... strs)
    {
        return this->append_from(0, std::forward<ArgTypes>(strs)...);
    }
    template <typename... ArgTypes>
    inline char*
    append(ArgTypes&&... strs)
    {
        return this->append_from(strlen(this->m_ptr),
                                 std::forward<ArgTypes>(strs)...);
    }
    template <typename... ArgTypes>
    inline char*
    append_from(size_t orig_len, ArgTypes&&... strs)
    {
        const std::array<const char*, sizeof...(strs)> strs_l{{strs...}};
        const std::array<size_t, sizeof...(strs)> str_lens{{strlen(strs)...}};
        const size_t total_len = std::accumulate(str_lens.begin(),
                                                 str_lens.end(), 0) + orig_len;
        this->resize(total_len);
        char *p = this->m_ptr + orig_len;
        for (size_t i = 0;i < sizeof...(strs);i++) {
            memcpy(p, strs_l[i], str_lens[i]);
            p += str_lens[i];
        }
        this->m_ptr[total_len] = 0;
        return this->m_ptr;
    }
};

template<typename T>
T convert(const char*, const T &def=T(), bool *is_def=nullptr);

#ifndef __QTC_UTILS_STRS_INTERNAL__
extern template double convert<double>(const char*, const double& =0,
                                       bool* =nullptr);
extern template long convert<long>(const char*, const long& =0, bool* =nullptr);
extern template bool convert<bool>(const char*, const bool& =false,
                                   bool* =nullptr);
#endif

template<>
inline std::string
convert<std::string>(const char *str, const std::string&, bool *is_def)
{
    qtcAssign(is_def, false);
    return std::string(str);
}

template<>
inline char*
convert<char*>(const char *str, char *const&, bool *is_def)
{
    qtcAssign(is_def, false);
    return strdup(str);
}

}

namespace StrList {

void _forEach(const char *str, char delim, char escape,
              const std::function<bool(const char*, size_t)> &func);

template<typename Func, typename... Args>
static inline auto
_forEachCaller(const char *str, size_t len, Func &func, Args&... args)
    -> typename std::enable_if<std::is_same<decltype(func(str, len, args...)),
                                            bool>::value, bool>::type
{
    return func(str, len, args...);
}

template<typename Func, typename... Args>
static inline auto
_forEachCaller(const char *str, size_t, Func &func, Args&... args)
    -> typename std::enable_if<std::is_same<decltype(func(str, args...)),
                                            bool>::value, bool>::type
{
    return func(str, args...);
}

template<typename Func, typename... Args>
static inline void
forEach(const char *str, char delim, char escape, Func &&func, Args&&... args)
{
    using namespace std::placeholders;
    _forEach(str, delim, escape,
             std::bind(_forEachCaller<typename std::decay<Func>::type,
                       typename std::decay<Args>::type...>, _1, _2,
                       std::forward<Func>(func), std::forward<Args>(args)...));
}
template<typename Func, typename... Args>
static inline typename std::enable_if<!_isChar<Func>::value>::type
forEach(const char *str, char delim, Func &&func, Args&&... args)
{
    forEach(str, delim, '\\', std::forward<Func>(func),
            std::forward<Args>(args)...);
}
template<typename Func, typename... Args>
static inline typename std::enable_if<!_isChar<Func>::value>::type
forEach(const char *str, Func &&func, Args&&... args)
{
    forEach(str, ',', std::forward<Func>(func), std::forward<Args>(args)...);
}

}
}

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

#endif
