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

namespace QtCurve {
namespace ScrolledWindow {

static void
cleanup(GtkWidget *widget)
{
    GtkWidgetProps props(widget);
    if (widget && props->scrolledWindowHacked) {
        props->scrolledWindowDestroy.disconn();
        props->scrolledWindowUnrealize.disconn();
        props->scrolledWindowStyleSet.disconn();
        if (opts.unifyCombo && opts.unifySpin) {
            props->scrolledWindowEnter.disconn();
            props->scrolledWindowLeave.disconn();
        }
        props->scrolledWindowFocusIn.disconn();
        props->scrolledWindowFocusOut.disconn();
        props->scrolledWindowHacked = false;
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

static GtkWidget *focusWidget = nullptr;
static GtkWidget *hoverWidget = nullptr;

static gboolean
enter(GtkWidget *widget, GdkEventMotion*, void *data)
{
    GtkWidget *w = data ? (GtkWidget*)data : widget;
    if (GTK_IS_SCROLLED_WINDOW(w) && hoverWidget != w) {
        hoverWidget = w;
        gtk_widget_queue_draw(w);
    }
    return false;
}

static gboolean
leave(GtkWidget *widget, GdkEventMotion*, void *data)
{
    GtkWidget *w = data ? (GtkWidget*)data : widget;
    if (GTK_IS_SCROLLED_WINDOW(w) && hoverWidget == w) {
        hoverWidget = nullptr;
        gtk_widget_queue_draw(w);
    }
    return false;
}

static gboolean
focusIn(GtkWidget *widget, GdkEventMotion*, void *data)
{
    GtkWidget *w = data ? (GtkWidget*)data : widget;
    if (GTK_IS_SCROLLED_WINDOW(w) && focusWidget != w) {
        focusWidget = w;
        gtk_widget_queue_draw(w);
    }
    return false;
}

static gboolean
focusOut(GtkWidget *widget, GdkEventMotion*, void *data)
{
    GtkWidget *w = data ? (GtkWidget*)data : widget;
    if (GTK_IS_SCROLLED_WINDOW(w) && focusWidget == w) {
        focusWidget = nullptr;
        gtk_widget_queue_draw(w);
    }
    return false;
}

static void
setupConnections(GtkWidget *widget, GtkWidget *parent)
{
    GtkWidgetProps props(widget);
    if (widget && !props->scrolledWindowHacked) {
        props->scrolledWindowHacked = true;
        gtk_widget_add_events(widget, GDK_LEAVE_NOTIFY_MASK |
                              GDK_ENTER_NOTIFY_MASK | GDK_FOCUS_CHANGE_MASK);
        props->scrolledWindowDestroy.conn("destroy-event", destroy, parent);
        props->scrolledWindowUnrealize.conn("unrealize", destroy, parent);
        props->scrolledWindowStyleSet.conn("style-set", styleSet, parent);
        if (opts.unifyCombo && opts.unifySpin) {
            props->scrolledWindowEnter.conn("enter-notify-event",
                                            enter, parent);
            props->scrolledWindowLeave.conn("leave-notify-event",
                                            leave, parent);
        }
        props->scrolledWindowFocusIn.conn("focus-in-event", focusIn, parent);
        props->scrolledWindowFocusOut.conn("focus-out-event", focusOut, parent);
        if (parent && opts.unifyCombo && opts.unifySpin) {
            QtcRect alloc = Widget::getAllocation(parent);
            int x;
            int y;
            gdk_window_get_pointer(gtk_widget_get_window(parent),
                                   &x, &y, nullptr);
            if (x >= 0 && x <alloc.width && y >= 0 && y < alloc.height) {
                hoverWidget = parent;
            }
        }
    }
}

void
setup(GtkWidget *widget)
{
    GtkWidgetProps props(widget);
    if (widget && GTK_IS_SCROLLED_WINDOW(widget) &&
        !props->scrolledWindowHacked) {
        GtkScrolledWindow *scrolledWindow = GTK_SCROLLED_WINDOW(widget);
        GtkWidget *child;

        if ((child = gtk_scrolled_window_get_hscrollbar(scrolledWindow))) {
            setupConnections(child, widget);
        }
        if ((child = gtk_scrolled_window_get_vscrollbar(scrolledWindow))) {
            setupConnections(child, widget);
        }
        if ((child = gtk_bin_get_child(GTK_BIN(widget)))) {
            if (GTK_IS_TREE_VIEW(child) || GTK_IS_TEXT_VIEW(child) ||
                GTK_IS_ICON_VIEW(child)) {
                setupConnections(child, widget);
            } else {
                const char *type = g_type_name(G_OBJECT_TYPE(child));
                if (type && (strcmp(type, "ExoIconView") == 0 ||
                             strcmp(type, "FMIconContainer") == 0)) {
                    setupConnections(child, widget);
                }
            }
        }
        props->scrolledWindowHacked = true;
    }
}

void
registerChild(GtkWidget *child)
{
    GtkWidget *parent = child ? gtk_widget_get_parent(child) : nullptr;

    GtkWidgetProps parentProps(parent);
    if (parent && GTK_IS_SCROLLED_WINDOW(parent) &&
        parentProps->scrolledWindowHacked) {
        setupConnections(child, parent);
    }
}

bool
hasFocus(GtkWidget *widget)
{
    return widget && (gtk_widget_has_focus(widget) || widget == focusWidget);
}

bool
hovered(GtkWidget *widget)
{
    return widget && (gtk_widget_get_state(widget) == GTK_STATE_PRELIGHT ||
                      widget == hoverWidget);
}

}
}
