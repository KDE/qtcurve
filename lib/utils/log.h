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

namespace QtCurve {

void backtrace();
enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
    Force
};

namespace Log {

LogLevel level();

__attribute__((format(printf, 5, 0)))
void logv(LogLevel level, const char *fname, int line,
          const char *func, const char *fmt, va_list ap);

__attribute__((format(printf, 5, 6)))
void log(LogLevel level, const char *fname, int line, const char *func,
         const char *fmt, ...);

static inline bool
checkLevel(LogLevel _level)
{
    return qtcUnlikely(_level <= LogLevel::Force && _level >= level());
}

}
}

#define qtcLog(__level, fmt, args...)                                   \
    do {                                                                \
        using namespace QtCurve;                                        \
        LogLevel level = (__level);                                     \
        if (!Log::checkLevel(level)) {                                  \
            break;                                                      \
        }                                                               \
        Log::log(level, __FILE__, __LINE__, __FUNCTION__, fmt, ##args); \
    } while (0)

#define qtcDebug(fmt, args...) qtcLog(LogLevel::Debug, fmt, ##args)
#define qtcInfo(fmt, args...) qtcLog(LogLevel::Info, fmt, ##args)
#define qtcWarn(fmt, args...) qtcLog(LogLevel::Warn, fmt, ##args)
#define qtcError(fmt, args...) qtcLog(LogLevel::Error, fmt, ##args)
#define qtcForceLog(fmt, args...) qtcLog(LogLevel::Force, fmt, ##args)

#endif
