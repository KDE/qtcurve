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

namespace QtCurve {

QTC_EXPORT const char*
getProgName()
{
    static uniqueCPtr<char> name([] {
            void *hdl = dlopen(nullptr, RTLD_NOW);
            // I do not use the procfs here since any system that has procfs
            // (Linux) should support one of these variables/functions.
            if (!hdl) {
                return strdup("");
            }
            void *progname_p = nullptr;
            const char *name = nullptr;
            if ((progname_p = dlsym(hdl, "program_invocation_short_name")) ||
                (progname_p = dlsym(hdl, "program_invocation_name")) ||
                (progname_p = dlsym(hdl, "__progname"))) {
                name = *(char *const*)progname_p;
            } else if ((progname_p = dlsym(hdl, "getprogname"))) {
                name = ((const char *(*)())progname_p)();
            }
            if (!name) {
                return strdup("");
            }
            const char *sub_name;
            if ((sub_name = strrchr(name, '/')) && sub_name[1]) {
                return strdup(sub_name + 1);
            }
            return strdup(name);
        }());
    return name.get();
}

}
