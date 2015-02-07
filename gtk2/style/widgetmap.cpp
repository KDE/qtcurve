/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
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

#include "widgetmap.h"

#include <qtcurve-utils/gtkprops.h>

namespace QtCurve {
namespace WidgetMap {

static GHashTable *hashTable[2] = {nullptr, nullptr};

template<typename Props, typename Id>
static inline bool
getMapHacked(Props &props, Id &id)
{
    return qtcWidgetProps(props)->widgetMapHacked & (id ? (1 << 1) : (1 << 0));
}

template<typename Props, typename Id>
static inline void
setMapHacked(Props &props, Id &id)
{
    qtcWidgetProps(props)->widgetMapHacked |= id ? (1 << 1) : (1 << 0);
}

static GtkWidget*
lookupHash(void *hash, void *value, int map)
{
    GtkWidget *rv = nullptr;

    if (!hashTable[map])
        hashTable[map] =
            g_hash_table_new(g_direct_hash, g_direct_equal);

    rv = (GtkWidget*)g_hash_table_lookup(hashTable[map], hash);

    if (!rv && value) {
        g_hash_table_insert(hashTable[map], hash, value);
        rv = (GtkWidget*)value;
    }
    return rv;
}

static void
removeHash(void *hash)
{
    for (int i = 0;i < 2;++i) {
        if (hashTable[i]) {
            g_hash_table_remove(hashTable[i], hash);
        }
    }
}

static void
cleanup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (qtcWidgetProps(props)->widgetMapHacked) {
        qtcDisconnectFromProp(props, widgetMapDestroy);
        qtcDisconnectFromProp(props, widgetMapUnrealize);
        qtcDisconnectFromProp(props, widgetMapStyleSet);
        qtcWidgetProps(props)->widgetMapHacked = 0;
        removeHash(widget);
    }
}

static gboolean
styleSet(GtkWidget *widget, GtkStyle*, void*)
{
    cleanup(widget);
    return false;
}

static gboolean
destroy(GtkWidget *widget, GdkEvent*, void*)
{
    cleanup(widget);
    return false;
}

void
setup(GtkWidget *from, GtkWidget *to, int map)
{
    QTC_DEF_WIDGET_PROPS(fromProps, from);
    if (from && to && !getMapHacked(fromProps, map)) {
        if (!qtcWidgetProps(fromProps)->widgetMapHacked) {
            qtcConnectToProp(fromProps, widgetMapDestroy, "destroy-event",
                             destroy, nullptr);
            qtcConnectToProp(fromProps, widgetMapUnrealize, "unrealize",
                             destroy, nullptr);
            qtcConnectToProp(fromProps, widgetMapStyleSet, "style-set",
                             styleSet, nullptr);
        }
        setMapHacked(fromProps, map);
        lookupHash(from, to, map);
    }
}

GtkWidget*
getWidget(GtkWidget *widget, int map)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    return (widget && getMapHacked(props, map) ?
            lookupHash(widget, nullptr, map) : nullptr);
}

}
}
