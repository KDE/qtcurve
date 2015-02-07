/*****************************************************************************
 *   Copyright 2015 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef __QTCURVE_GTK_GDBUS_H__
#define __QTCURVE_GTK_GDBUS_H__

#include <gio/gio.h>
#include <stdint.h>
#include <utility>

namespace QtCurve {
namespace GDBus {

GDBusConnection *getConnection();
void _callMethod(const char *bus_name, const char *path, const char *iface,
                 const char *method, GVariant *params);

#define DEF_GVARIANT_CONVERT(type, gname)       \
    static inline GVariant*                     \
    toGVariant(type val)                        \
    {                                           \
        return g_variant_new_##gname(val);      \
    }

DEF_GVARIANT_CONVERT(bool, boolean)

DEF_GVARIANT_CONVERT(signed char, byte)
DEF_GVARIANT_CONVERT(unsigned char, byte)

DEF_GVARIANT_CONVERT(int16_t, int16)
DEF_GVARIANT_CONVERT(uint16_t, uint16)

DEF_GVARIANT_CONVERT(int32_t, int32)
DEF_GVARIANT_CONVERT(uint32_t, uint32)

DEF_GVARIANT_CONVERT(int64_t, int64)
DEF_GVARIANT_CONVERT(uint64_t, uint64)

DEF_GVARIANT_CONVERT(double, double)

DEF_GVARIANT_CONVERT(const char*, string)

static inline GVariant*
toGVariant(GVariant *val)
{
    return val;
}

#undef DEF_GVARIANT_CONVERT

template<typename... Args>
static inline void
callMethod(const char *bus_name, const char *path, const char *iface,
           const char *method, Args&&... args)
{
    GVariant *g_args[] = {toGVariant(std::forward<Args>(args))...};
    _callMethod(bus_name, path, iface, method,
                g_variant_new_tuple(g_args, sizeof...(args)));
}

}
}

#endif
