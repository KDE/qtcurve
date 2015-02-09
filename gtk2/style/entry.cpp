/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#include "entry.h"

#include <qtcurve-utils/gtkprops.h>

namespace QtCurve {
namespace Entry {

static GtkWidget *lastMo = nullptr;

static void
cleanup(GtkWidget *widget)
{
    if (lastMo == widget) {
        lastMo = nullptr;
    }
    if (GTK_IS_ENTRY(widget)) {
        QTC_DEF_WIDGET_PROPS(props, widget);
        qtcDisconnectFromProp(props, entryEnter);
        qtcDisconnectFromProp(props, entryLeave);
        qtcDisconnectFromProp(props, entryDestroy);
        qtcDisconnectFromProp(props, entryUnrealize);
        qtcDisconnectFromProp(props, entryStyleSet);
        qtcWidgetProps(props)->entryHacked = false;
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

static gboolean
enter(GtkWidget *widget, GdkEventCrossing*, void*)
{
    if (GTK_IS_ENTRY(widget)) {
        lastMo = widget;
        gtk_widget_queue_draw(widget);
    }
    return false;
}

static gboolean
leave(GtkWidget *widget, GdkEventCrossing*, void*)
{
    if (GTK_IS_ENTRY(widget)) {
        lastMo = nullptr;
        gtk_widget_queue_draw(widget);
    }
    return false;
}

bool
isLastMo(GtkWidget *widget)
{
    return lastMo && widget == lastMo;
}

void
setup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (GTK_IS_ENTRY(widget) && !qtcWidgetProps(props)->entryHacked) {
        qtcWidgetProps(props)->entryHacked = true;
        qtcConnectToProp(props, entryEnter, "enter-notify-event",
                         enter, nullptr);
        qtcConnectToProp(props, entryLeave, "leave-notify-event",
                         leave, nullptr);
        qtcConnectToProp(props, entryDestroy, "destroy-event",
                         destroy, nullptr);
        qtcConnectToProp(props, entryUnrealize, "unrealize",
                         destroy, nullptr);
        qtcConnectToProp(props, entryStyleSet, "style-set",
                         styleSet, nullptr);
    }
}

}
}
