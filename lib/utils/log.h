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

#ifndef _QTC_UTILS_LOG_H_
#define _QTC_UTILS_LOG_H_

#include "utils.h"
#include <stdarg.h>

typedef enum {
    QTC_LOG_DEBUG,
    QTC_LOG_INFO,
    QTC_LOG_WARN,
    QTC_LOG_ERROR,
    QTC_LOG_FORCE
} QtcLogLevel;

QtcLogLevel _qtcGetLogLevel();
bool _qtcGetLogColor();

static inline QtcLogLevel
qtcGetLogLevel()
{
    static bool inited = false;
    static QtcLogLevel level = QTC_LOG_ERROR;
    if (!inited) {
        level = _qtcGetLogLevel();
        inited = true;
    }
    return level;
}

static inline bool
qtcGetLogColor()
{
    static bool inited = false;
    static bool color = false;
    if (!inited) {
        color = _qtcGetLogColor();
        inited = true;
    }
    return color;
}

#define qtcLogLevel (qtcGetLogLevel())
#define qtcLogColor (qtcGetLogColor())

static inline bool
qtcCheckLogLevel(unsigned level)
{
    return qtcUnlikely(level <= QTC_LOG_FORCE && level >= qtcLogLevel);
}

__attribute__((format(printf, 5, 6)))
void _qtcLog(QtcLogLevel level, const char *fname, int line, const char *func,
             const char *fmt, ...);

__attribute__((format(printf, 5, 0)))
void _qtcLogV(QtcLogLevel level, const char *fname, int line, const char *func,
              const char *fmt, va_list ap);

#define qtcLog(__level, fmt, args...)                                   \
    do {                                                                \
        unsigned level = (__level);                                     \
        if (!qtcCheckLogLevel(level)) {                                 \
            break;                                                      \
        }                                                               \
        _qtcLog((QtcLogLevel)level, __FILE__, __LINE__, __FUNCTION__,   \
                fmt, ##args);                                           \
    } while (0)

#define qtcDebug(fmt, args...)                  \
    qtcLog(QTC_LOG_DEBUG, fmt, ##args)
#define qtcInfo(fmt, args...)                   \
    qtcLog(QTC_LOG_INFO, fmt, ##args)
#define qtcWarn(fmt, args...)                   \
    qtcLog(QTC_LOG_WARN, fmt, ##args)
#define qtcError(fmt, args...)                  \
    qtcLog(QTC_LOG_ERROR, fmt, ##args)
#define qtcForceLog(fmt, args...)               \
    qtcLog(QTC_LOG_FORCE, fmt, ##args)

void qtcBacktrace();

#endif
