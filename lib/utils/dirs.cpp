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

#include "dirs.h"
#include "log.h"
#include "strs.h"
#include <sys/types.h>
#include <pwd.h>

static char *qtc_home = NULL;
static char *qtc_xdg_data_home = NULL;
static char *qtc_xdg_config_home = NULL;
static char *qtc_conf_dir = NULL;

QTC_EXPORT void
qtcMakePath(const char *path, int mode)
{
    if (qtcIsDir(path)) {
        return;
    }
    size_t len = strlen(path);
    char opath[len + 1];
    memcpy(opath, path, len + 1);
    while (opath[len - 1] == '/') {
        opath[len - 1] = '\0';
        len--;
    }
    char *p = opath + strspn(opath, "/");
    if (!p) {
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

static const char*
_qtcConfDir()
{
    if (qtcUnlikely(!qtc_conf_dir)) {
        const char *env_home = getenv("QTCURVE_CONFIG_DIR");
        if (env_home && *env_home == '/') {
            qtc_conf_dir = QtCurve::catStrs(env_home, "/");
        } else {
            qtc_conf_dir = QtCurve::catStrs(qtcGetXDGConfigHome(), "qtcurve/");
        }
    }
    return qtc_conf_dir;
}

QTC_EXPORT const char*
qtcConfDir()
{
    static const char *conf_dir = NULL;
    if (!conf_dir) {
        conf_dir = _qtcConfDir();
        qtcMakePath(conf_dir, 0700);
    }
    return conf_dir;
}

QTC_EXPORT const char*
qtcGetHome()
{
    if (qtcUnlikely(!qtc_home)) {
        const char *env_home = getenv("HOME");
        if (qtcLikely(env_home && *env_home == '/')) {
            qtc_home = QtCurve::catStrs(env_home, "/");
        } else {
            struct passwd *pw = getpwuid(getuid());
            if (qtcLikely(pw && pw->pw_dir && *pw->pw_dir == '/')) {
                qtc_home = QtCurve::catStrs(pw->pw_dir, "/");
            }
        }
        if (qtcUnlikely(!qtc_home)) {
            qtc_home = strdup("/tmp/");
        }
    }
    return qtc_home;
}

QTC_EXPORT const char*
qtcGetXDGDataHome()
{
    if (qtcUnlikely(!qtc_xdg_data_home)) {
        const char *env_home = getenv("XDG_DATA_HOME");
        if (env_home && *env_home == '/') {
            qtc_xdg_data_home = QtCurve::catStrs(env_home, "/");
        } else {
            qtc_xdg_data_home = QtCurve::catStrs(qtcGetHome(), ".local/share/");
        }
    }
    return qtc_xdg_data_home;
}

QTC_EXPORT const char*
qtcGetXDGConfigHome()
{
    if (qtcUnlikely(!qtc_xdg_config_home)) {
        const char *env_home = getenv("XDG_CONFIG_HOME");
        if (env_home && *env_home == '/') {
            qtc_xdg_config_home = QtCurve::catStrs(env_home, "/");
        } else {
            qtc_xdg_config_home = QtCurve::catStrs(qtcGetHome(), ".config/");
        }
    }
    return qtc_xdg_config_home;
}

QTC_EXPORT char*
qtcGetConfFile(const char *file, char *buff)
{
    if (file[0] == '/') {
        return QtCurve::fillStrs(buff, file);
    }
    return QtCurve::fillStrs(buff, qtcConfDir(), file);
}

__attribute__((constructor)) static void
_qtcGetDirs()
{
    qtcGetHome();
    qtcGetXDGDataHome();
    qtcGetXDGConfigHome();
    _qtcConfDir();
}

__attribute__((destructor)) static void
_qtcFreeDirs()
{
    free(qtc_home);
    qtc_home = NULL;
    free(qtc_xdg_data_home);
    qtc_xdg_data_home = NULL;
    free(qtc_xdg_config_home);
    qtc_xdg_config_home = NULL;
    free(qtc_conf_dir);
    qtc_conf_dir = NULL;
}
