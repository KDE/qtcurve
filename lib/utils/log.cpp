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

#include <config.h>
#include "log.h"
#include "strs.h"
#include "map.h"
#include <unistd.h>
#include <stdarg.h>

#ifdef QTC_ENABLE_BACKTRACE
#include <execinfo.h>
#endif

namespace QtCurve {

QTC_EXPORT void
backtrace()
{
#ifdef QTC_ENABLE_BACKTRACE
    void *buff[1024];
    size_t size = ::backtrace(buff, 1024);
    ::backtrace_symbols_fd(buff, size, STDERR_FILENO);
#endif
}

namespace Log {

static inline LogLevel
getLevel()
{
    const char *env_debug = getenv("QTCURVE_DEBUG");
    if (Str::convert(env_debug, false)) {
        return LogLevel::Debug;
    }
    static const StrMap<LogLevel, false> level_map{
        {"debug", LogLevel::Debug},
        {"info", LogLevel::Info},
        {"warning", LogLevel::Warn},
        {"warn", LogLevel::Warn},
        {"error", LogLevel::Error}
    };
    LogLevel res = level_map.search(getenv("QTCURVE_LEVEL"), LogLevel::Error);
    if (Str::convert(env_debug, true) && res <= LogLevel::Debug) {
        return LogLevel::Info;
    }
    return res;
}

static inline bool
getUseColor()
{
    const char *env_color = getenv("QTCURVE_LOG_COLOR");
    if (Str::convert(env_color, false)) {
        return true;
    } else if (!Str::convert(env_color, true)) {
        return false;
    } else if (isatty(2)) {
        return true;
    } else {
        return false;
    }
}

QTC_EXPORT LogLevel
level()
{
    static LogLevel _level = Log::getLevel();
    return _level;
}

static bool
useColor()
{
    static bool color = Log::getUseColor();
    return color;
}

QTC_EXPORT void
logv(LogLevel _level, const char *fname, int line, const char *func,
     const char *fmt, va_list ap)
{
    QTC_RET_IF_FAIL(_level >= level() && ((int)_level) >= 0 &&
                    _level <= LogLevel::Force);
    static const char *color_codes[] = {
        [(int)LogLevel::Debug] = "\e[01;32m",
        [(int)LogLevel::Info] = "\e[01;34m",
        [(int)LogLevel::Warn] = "\e[01;33m",
        [(int)LogLevel::Error] = "\e[01;31m",
        [(int)LogLevel::Force] = "\e[01;35m",
    };

    static const char *log_prefixes[] = {
        [(int)LogLevel::Debug] = "qtcDebug-",
        [(int)LogLevel::Info] = "qtcInfo-",
        [(int)LogLevel::Warn] = "qtcWarn-",
        [(int)LogLevel::Error] = "qtcError-",
        [(int)LogLevel::Force] = "qtcLog-",
    };

    const char *color_prefix = (useColor() ? color_codes[(int)_level] : "");
    const char *log_prefix = log_prefixes[(int)_level];

    fprintf(stderr, "%s%s%d (%s:%d) %s ", color_prefix, log_prefix, getpid(),
            fname, line, func);
    vfprintf(stderr, fmt, ap);
    if (useColor()) {
        fwrite("\e[0m", strlen("\e[0m"), 1, stderr);
    }
}

QTC_EXPORT void
log(LogLevel _level, const char *fname, int line, const char *func,
    const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logv(_level, fname, line, func, fmt, ap);
    va_end(ap);
}

}
}
