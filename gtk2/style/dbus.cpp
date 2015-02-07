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

#include "dbus.h"

namespace QtCurve {
namespace GDBus {

GDBusConnection*
getConnection()
{
    static GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION,
                                                  nullptr, nullptr);
    return conn;
}

void
_callMethod(const char *bus_name, const char *path, const char *iface,
            const char *method, GVariant *params)
{
    g_dbus_connection_call(getConnection(), bus_name, path, iface, method,
                           params, nullptr, G_DBUS_CALL_FLAGS_NONE, -1,
                           nullptr, nullptr, nullptr);
}

}
}
