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

#include "scrolledwindow.h"

#include "qt_settings.h"

#include <qtcurve-utils/gtkprops.h>
#include <qtcurve-cairo/utils.h>
#include <common/common.h>

static void
qtcScrolledWindowCleanup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && qtcWidgetProps(props)->scrolledWindowHacked) {
        qtcDisconnectFromProp(props, scrolledWindowDestroy);
        qtcDisconnectFromProp(props, scrolledWindowUnrealize);
        qtcDisconnectFromProp(props, scrolledWindowStyleSet);
        if (QtCurve::opts.unifyCombo && QtCurve::opts.unifySpin) {
            qtcDisconnectFromProp(props, scrolledWindowEnter);
            qtcDisconnectFromProp(props, scrolledWindowLeave);
        }
        qtcDisconnectFromProp(props, scrolledWindowFocusIn);
        qtcDisconnectFromProp(props, scrolledWindowFocusOut);
        qtcWidgetProps(props)->scrolledWindowHacked = false;
    }
}

static gboolean
qtcScrolledWindowStyleSet(GtkWidget *widget, GtkStyle*, void*)
{
    qtcScrolledWindowCleanup(widget);
    return false;
}

static gboolean
qtcScrolledWindowDestroy(GtkWidget *widget, GdkEvent*, void*)
{
    qtcScrolledWindowCleanup(widget);
    return false;
}

static GtkWidget *qtcScrolledWindowFocus = NULL;
static GtkWidget *qtcScrolledWindowHover = NULL;

gboolean qtcScrolledWindowHasFocus(GtkWidget *widget)
{
    return widget && (gtk_widget_has_focus(widget) ||
                      widget == qtcScrolledWindowFocus);
}

gboolean qtcScrolledWindowHovered(GtkWidget *widget)
{
    return widget && (gtk_widget_get_state(widget) == GTK_STATE_PRELIGHT ||
                      widget == qtcScrolledWindowHover);
}

static gboolean
qtcScrolledWindowEnter(GtkWidget *widget, GdkEventMotion*, void *data)
{
    GtkWidget *w = data ? (GtkWidget*)data : widget;
    if (GTK_IS_SCROLLED_WINDOW(w) && qtcScrolledWindowHover != w) {
        qtcScrolledWindowHover = w;
        gtk_widget_queue_draw(w);
    }
    return false;
}

static gboolean
qtcScrolledWindowLeave(GtkWidget *widget, GdkEventMotion*, void *data)
{
    GtkWidget *w = data ? (GtkWidget*)data : widget;
    if (GTK_IS_SCROLLED_WINDOW(w) && qtcScrolledWindowHover == w) {
        qtcScrolledWindowHover = NULL;
        gtk_widget_queue_draw(w);
    }
    return false;
}

static gboolean
qtcScrolledWindowFocusIn(GtkWidget *widget, GdkEventMotion*, void *data)
{
    GtkWidget *w = data ? (GtkWidget*)data : widget;
    if (GTK_IS_SCROLLED_WINDOW(w) && qtcScrolledWindowFocus != w) {
        qtcScrolledWindowFocus = w;
        gtk_widget_queue_draw(w);
    }
    return false;
}

static gboolean
qtcScrolledWindowFocusOut(GtkWidget *widget, GdkEventMotion*, void *data)
{
    GtkWidget *w = data ? (GtkWidget*)data : widget;
    if (GTK_IS_SCROLLED_WINDOW(w) && qtcScrolledWindowFocus == w) {
        qtcScrolledWindowFocus = NULL;
        gtk_widget_queue_draw(w);
    }
    return false;
}

static void
qtcScrolledWindowSetupConnections(GtkWidget *widget, GtkWidget *parent)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->scrolledWindowHacked) {
        qtcWidgetProps(props)->scrolledWindowHacked = true;
        gtk_widget_add_events(widget, GDK_LEAVE_NOTIFY_MASK |
                              GDK_ENTER_NOTIFY_MASK | GDK_FOCUS_CHANGE_MASK);
        qtcConnectToProp(props, scrolledWindowDestroy,
                         "destroy-event", qtcScrolledWindowDestroy, parent);
        qtcConnectToProp(props, scrolledWindowUnrealize,
                         "unrealize", qtcScrolledWindowDestroy, parent);
        qtcConnectToProp(props, scrolledWindowStyleSet,
                         "style-set", qtcScrolledWindowStyleSet, parent);
        if (QtCurve::opts.unifyCombo && QtCurve::opts.unifySpin) {
            qtcConnectToProp(props, scrolledWindowEnter, "enter-notify-event",
                             qtcScrolledWindowEnter, parent);
            qtcConnectToProp(props, scrolledWindowLeave, "leave-notify-event",
                             qtcScrolledWindowLeave, parent);
        }
        qtcConnectToProp(props, scrolledWindowFocusIn, "focus-in-event",
                         qtcScrolledWindowFocusIn, parent);
        qtcConnectToProp(props, scrolledWindowFocusOut, "focus-out-event",
                         qtcScrolledWindowFocusOut, parent);
        if (parent && QtCurve::opts.unifyCombo && QtCurve::opts.unifySpin) {
            int x, y;
            QtcRect alloc = qtcWidgetGetAllocation(parent);

            gdk_window_get_pointer(gtk_widget_get_window(parent), &x, &y, 0L);
            if (x >= 0 && x <alloc.width && y >= 0 && y < alloc.height) {
                qtcScrolledWindowHover = parent;
            }
        }
    }
}

void
qtcScrolledWindowRegisterChild(GtkWidget *child)
{
    GtkWidget *parent = child ? gtk_widget_get_parent(child) : NULL;

    QTC_DEF_WIDGET_PROPS(parentProps, parent);
    if (parent && GTK_IS_SCROLLED_WINDOW(parent) &&
        qtcWidgetProps(parentProps)->scrolledWindowHacked) {
        qtcScrolledWindowSetupConnections(child, parent);
    }
}

void
qtcScrolledWindowSetup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && GTK_IS_SCROLLED_WINDOW(widget) &&
        !qtcWidgetProps(props)->scrolledWindowHacked) {
        GtkScrolledWindow *scrolledWindow = GTK_SCROLLED_WINDOW(widget);
        GtkWidget *child;

        if ((child = gtk_scrolled_window_get_hscrollbar(scrolledWindow))) {
            qtcScrolledWindowSetupConnections(child, widget);
        }
        if ((child = gtk_scrolled_window_get_vscrollbar(scrolledWindow))) {
            qtcScrolledWindowSetupConnections(child, widget);
        }
        if ((child = gtk_bin_get_child(GTK_BIN(widget)))) {
            if (GTK_IS_TREE_VIEW(child) || GTK_IS_TEXT_VIEW(child) ||
                GTK_IS_ICON_VIEW(child)) {
                qtcScrolledWindowSetupConnections(child, widget);
            } else {
                const char *type = g_type_name(G_OBJECT_TYPE(child));
                if (type && (strcmp(type, "ExoIconView") == 0 ||
                             strcmp(type, "FMIconContainer") == 0)) {
                    qtcScrolledWindowSetupConnections(child, widget);
                }
            }
        }
        qtcWidgetProps(props)->scrolledWindowHacked = true;
    }
}
