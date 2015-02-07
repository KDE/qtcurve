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

#include "scrollbar.h"

#include <qtcurve-utils/gtkprops.h>

static void
qtcScrollbarCleanup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && qtcWidgetProps(props)->scrollBarHacked) {
        qtcDisconnectFromProp(props, scrollBarDestroy);
        qtcDisconnectFromProp(props, scrollBarUnrealize);
        qtcDisconnectFromProp(props, scrollBarStyleSet);
        qtcDisconnectFromProp(props, scrollBarValueChanged);
        qtcWidgetProps(props)->scrollBarHacked = false;
    }
}

static gboolean
qtcScrollbarStyleSet(GtkWidget *widget, GtkStyle*, void*)
{
    qtcScrollbarCleanup(widget);
    return false;
}

static gboolean
qtcScrollbarDestroy(GtkWidget *widget, GdkEvent*, void*)
{
    qtcScrollbarCleanup(widget);
    return false;
}

static GtkScrolledWindow*
qtcScrollbarParentScrolledWindow(GtkWidget *widget)
{
    GtkWidget *parent = widget;

    while (parent && (parent = gtk_widget_get_parent(parent))) {
        if (GTK_IS_SCROLLED_WINDOW(parent)) {
            return GTK_SCROLLED_WINDOW(parent);
        }
    }
    return NULL;
}

static gboolean
qtcScrollbarValueChanged(GtkWidget *widget, GdkEventMotion*, void*)
{
    if (GTK_IS_SCROLLBAR(widget)) {
        GtkScrolledWindow *sw = qtcScrollbarParentScrolledWindow(widget);

        if (sw) {
            gtk_widget_queue_draw(GTK_WIDGET(sw));
        }
    }
    return false;
}

static void
qtcScrollbarSetupSlider(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->scrollBarHacked) {
        qtcWidgetProps(props)->scrollBarHacked = true;
        qtcConnectToProp(props, scrollBarDestroy, "destroy-event",
                         qtcScrollbarDestroy, NULL);
        qtcConnectToProp(props, scrollBarUnrealize, "unrealize",
                         qtcScrollbarDestroy, NULL);
        qtcConnectToProp(props, scrollBarStyleSet, "style-set",
                         qtcScrollbarStyleSet, NULL);
        qtcConnectToProp(props, scrollBarValueChanged, "value-changed",
                         qtcScrollbarValueChanged, NULL);
    }
}

void
qtcScrollbarSetup(GtkWidget *widget)
{
    GtkScrolledWindow *sw = qtcScrollbarParentScrolledWindow(widget);

    if (sw) {
        GtkWidget *slider;

        if ((slider = gtk_scrolled_window_get_hscrollbar(sw))) {
            qtcScrollbarSetupSlider(slider);
        }
        if ((slider = gtk_scrolled_window_get_vscrollbar(sw))) {
            qtcScrollbarSetupSlider(slider);
        }
    }
}
