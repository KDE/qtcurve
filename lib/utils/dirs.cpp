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

#include "dirs.h"
#include "log.h"
#include "strs.h"

#include <config.h>

#include <sys/types.h>
#include <pwd.h>
#include <forward_list>

namespace QtCurve {

QTC_EXPORT void
makePath(const char *path, int mode)
{
    if (isDir(path)) {
        return;
    }
    Str::Buff<1024> opath(path);
    size_t len = opath.size() - 1;
    while (opath[len - 1] == '/') {
        opath[len - 1] = '\0';
        len--;
    }
    char *p = opath.get() + strspn(opath, "/");
    if (!*p) {
        return;
    }
    p += 1;
    for (;*p;p++) {
        if (*p == '/') {
            *p = '\0';
            if (access(opath, F_OK)) {
                mkdir(opath, mode | 0300);
            }
            *p = '/';
        }
    }
    if (access(opath, F_OK)) {
        mkdir(opath, mode);
    }
}

// Home
QTC_EXPORT const char*
getHome()
{
    static uniqueStr dir = [] {
        const char *env_home = getenv("HOME");
        if (qtcLikely(env_home && *env_home == '/')) {
            return Str::cat(env_home, "/");
        } else {
            struct passwd *pw = getpwuid(getuid());
            if (qtcLikely(pw && pw->pw_dir && *pw->pw_dir == '/')) {
                return Str::cat(pw->pw_dir, "/");
            }
        }
        return strdup("/tmp/");
    };
    return dir.get();
}

// XDG dirs
QTC_EXPORT const char*
getXDGConfigHome()
{
    static uniqueStr dir = [] {
        const char *env_home = getenv("XDG_CONFIG_HOME");
        if (env_home && *env_home == '/') {
            return Str::cat(env_home, "/");
        } else {
            return Str::cat(getHome(), ".config/");
        }
    };
    return dir.get();
}

QTC_EXPORT const char*
getXDGDataHome()
{
    static uniqueStr dir = [] {
        const char *env_home = getenv("XDG_DATA_HOME");
        if (env_home && *env_home == '/') {
            return Str::cat(env_home, "/");
        } else {
            return Str::cat(getHome(), ".local/share/");
        }
    };
    return dir.get();
}

// TODO
const std::forward_list<uniqueStr>&
getKDE4Home()
{
    static const std::forward_list<uniqueStr> homes = [] {
        std::forward_list<uniqueStr> res;
        auto add_dir = [&] (char *dir) {
            if (isDir(dir)) {
                res.emplace_front(dir);
            } else {
                free(dir);
            }
        };
        add_dir(Str::cat(getHome(), ".kde/"));
        add_dir(Str::cat(getHome(), ".kde4/"));
        char *env = getenv(getuid() ? "KDEHOME" : "KDEROOTHOME");
        if (env && env[0] == '/') {
            add_dir(Str::cat(env, "/"));
        } else {
#ifndef QTC_KDE4_DEFAULT_HOME_DEFAULT
            add_dir(Str::cat(getHome(), QTC_KDE4_DEFAULT_HOME "/"));
#endif
        }
        return res;
    }();
    return homes;
}

QTC_EXPORT const char*
getConfDir()
{
    static uniqueStr dir = [] {
        const char *env_home = getenv("QTCURVE_CONFIG_DIR");
        char *res = ((env_home && *env_home == '/') ? Str::cat(env_home, "/") :
                     Str::cat(getXDGConfigHome(), "qtcurve/"));
        makePath(res, 0700);
        return res;
    };
    return dir.get();
}

QTC_EXPORT char*
getConfFile(const char *file, char *buff)
{
    if (file[0] == '/')
        return Str::fill(buff, file);
    return Str::fill(buff, getConfDir(), file);
}

}
