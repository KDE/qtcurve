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

static GHashTable *qtcWidgetMapHashTable[2] = {nullptr, nullptr};
#define getMapHacked(props, id)                                         \
    (qtcWidgetProps(props)->widgetMapHacked & ((id) ? (1 << 1) : (1 << 0)))
#define setMapHacked(props, id)                                         \
    (qtcWidgetProps(props)->widgetMapHacked |= (id) ? (1 << 1) : (1 << 0))

static GtkWidget*
qtcWidgetMapLookupHash(void *hash, void *value, int map)
{
    GtkWidget *rv = nullptr;

    if (!qtcWidgetMapHashTable[map])
        qtcWidgetMapHashTable[map] =
            g_hash_table_new(g_direct_hash, g_direct_equal);

    rv = (GtkWidget*)g_hash_table_lookup(qtcWidgetMapHashTable[map], hash);

    if (!rv && value) {
        g_hash_table_insert(qtcWidgetMapHashTable[map], hash, value);
        rv = (GtkWidget*)value;
    }
    return rv;
}

static void
qtcWidgetMapRemoveHash(void *hash)
{
    for (int i = 0;i < 2;++i) {
        if (qtcWidgetMapHashTable[i]) {
            g_hash_table_remove(qtcWidgetMapHashTable[i], hash);
        }
    }
}

GtkWidget*
qtcWidgetMapGetWidget(GtkWidget *widget, int map)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    return (widget && getMapHacked(props, map) ?
            qtcWidgetMapLookupHash(widget, nullptr, map) : nullptr);
}

static void qtcWidgetMapCleanup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (qtcWidgetProps(props)->widgetMapHacked) {
        qtcDisconnectFromProp(props, widgetMapDestroy);
        qtcDisconnectFromProp(props, widgetMapUnrealize);
        qtcDisconnectFromProp(props, widgetMapStyleSet);
        qtcWidgetProps(props)->widgetMapHacked = 0;
        qtcWidgetMapRemoveHash(widget);
    }
}

static gboolean
qtcWidgetMapStyleSet(GtkWidget *widget, GtkStyle *prev_style, void *data)
{
    QTC_UNUSED(prev_style);
    QTC_UNUSED(data);
    qtcWidgetMapCleanup(widget);
    return false;
}

static gboolean
qtcWidgetMapDestroy(GtkWidget *widget, GdkEvent *event, void *user_data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(user_data);
    qtcWidgetMapCleanup(widget);
    return false;
}

void qtcWidgetMapSetup(GtkWidget *from, GtkWidget *to, int map)
{
    QTC_DEF_WIDGET_PROPS(fromProps, from);
    if (from && to && !getMapHacked(fromProps, map)) {
        if (!qtcWidgetProps(fromProps)->widgetMapHacked) {
            qtcConnectToProp(fromProps, widgetMapDestroy, "destroy-event",
                             qtcWidgetMapDestroy, nullptr);
            qtcConnectToProp(fromProps, widgetMapUnrealize, "unrealize",
                             qtcWidgetMapDestroy, nullptr);
            qtcConnectToProp(fromProps, widgetMapStyleSet, "style-set",
                             qtcWidgetMapStyleSet, nullptr);
        }
        setMapHacked(fromProps, map);
        qtcWidgetMapLookupHash(from, to, map);
    }
}
