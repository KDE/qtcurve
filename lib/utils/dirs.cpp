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

#include <forward_list>

#include <sys/types.h>
#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>

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

static const std::forward_list<std::string>&
getPresetDirs()
{
    static const std::forward_list<std::string> dirs = [] {
        std::forward_list<std::string> res;
        auto add_dir = [&] (const char *dir, bool needfree=true) {
            if (isDir(dir)) {
                res.emplace_front(dir);
            }
            if (needfree) {
                free(const_cast<char*>(dir));
            }
        };
        for (auto &kde_home: getKDE4Home()) {
            add_dir(Str::cat(kde_home.get(), "share/apps/QtCurve/"));
        }
        if (auto xdg_data_dirs = getenv("XDG_DATA_DIRS")) {
            while (true) {
                auto delim = strchr(xdg_data_dirs, ':');
                if (delim) {
                    auto origin = xdg_data_dirs;
                    xdg_data_dirs = delim + 1;
                    if (origin[0] != '/')
                        continue;
                    std::string dir(origin, delim - origin);
                    dir += "/QtCurve/";
                    if (isDir(dir.c_str())) {
                        res.push_front(std::move(dir));
                    }
                } else if (xdg_data_dirs[0] == '/') {
                    std::string dir(xdg_data_dirs);
                    dir += "/QtCurve/";
                    if (isDir(dir.c_str()))
                        res.push_front(std::move(dir));
                    break;
                } else {
                    break;
                }
            }
        } else {
            add_dir("/usr/local/share/QtCurve/", false);
            add_dir("/usr/share/QtCurve/", false);
        }
        auto xdg_home = Str::cat(getXDGDataHome(), "QtCurve/");
        makePath(xdg_home, 0700);
        add_dir(xdg_home);
        return res;
    }();
    return dirs;
}

QTC_EXPORT std::map<std::string, std::string>
getPresets()
{
    std::map<std::string, std::string> presets;

    for (auto &dir_name: getPresetDirs()) {
        DIR *dir = opendir(dir_name.c_str());
        if (!dir)
            continue;
        while (auto ent = readdir(dir)) {
            auto fname = ent->d_name;
            // Non preset files
            if (!Str::endsWith(fname, ".qtcurve"))
                continue;
            size_t name_len = strlen(fname) - strlen(".qtcurve");
            std::string name(fname, name_len);
            // preset already loaded
            if (presets.find(name) != presets.end())
                continue;
            presets[name] = dir_name + fname;
        }
        closedir(dir);
    }

    return presets;
}

}
