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

#include "utils.h"

#include <config.h>

#include <dlfcn.h>

const char*
_qtcGetProgName()
{
    static char buff[1024] = "";
    void *hdl = dlopen(nullptr, RTLD_NOW);
    const char *name = nullptr;
    char *const *progname = nullptr;
    const char *(*progname_func)() = nullptr;
    // I do not use the procfs here since any system that has procfs (Linux)
    // should support one of these variables/functions.
    if (!hdl) {
        return "";
    } else if ((progname =
                (char *const*)dlsym(hdl, "program_invocation_short_name")) ||
               (progname =
                (char *const*)dlsym(hdl, "program_invocation_name")) ||
               (progname = (char *const*)dlsym(hdl, "__progname"))) {
        name = *progname;
    } else if ((progname_func = (const char *(*)())dlsym(hdl, "getprogname"))) {
        name = progname_func();
    }
    if (!name) {
        return "";
    }
    strncpy(buff, name, 1024);
    buff[1023] = '\0';
    buff[strcspn(buff, " ")] = '\0';
    if (qtcUnlikely(name = strrchr(buff, '/'))) {
        name++;
        if (*name) {
            return name;
        }
    }
    return buff;
}

QTC_EXPORT const char*
qtcGetProgName()
{
    static const char *name = _qtcGetProgName();
    return name;
}
