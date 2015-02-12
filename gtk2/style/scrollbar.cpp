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

#include "scrollbar.h"

#include <qtcurve-utils/gtkprops.h>

namespace QtCurve {
namespace Scrollbar {

static void
cleanup(GtkWidget *widget)
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

static GtkScrolledWindow*
parentScrolledWindow(GtkWidget *widget)
{
    GtkWidget *parent = widget;

    while (parent && (parent = gtk_widget_get_parent(parent))) {
        if (GTK_IS_SCROLLED_WINDOW(parent)) {
            return GTK_SCROLLED_WINDOW(parent);
        }
    }
    return nullptr;
}

static gboolean
valueChanged(GtkWidget *widget, GdkEventMotion*, void*)
{
    if (GTK_IS_SCROLLBAR(widget)) {
        GtkScrolledWindow *sw = parentScrolledWindow(widget);

        if (sw) {
            gtk_widget_queue_draw(GTK_WIDGET(sw));
        }
    }
    return false;
}

static void
setupSlider(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->scrollBarHacked) {
        qtcWidgetProps(props)->scrollBarHacked = true;
        qtcConnectToProp(props, scrollBarDestroy, "destroy-event", destroy);
        qtcConnectToProp(props, scrollBarUnrealize, "unrealize", destroy);
        qtcConnectToProp(props, scrollBarStyleSet, "style-set", styleSet);
        qtcConnectToProp(props, scrollBarValueChanged, "value-changed",
                         valueChanged);
    }
}

void
setup(GtkWidget *widget)
{
    GtkScrolledWindow *sw = parentScrolledWindow(widget);

    if (sw) {
        GtkWidget *slider;

        if ((slider = gtk_scrolled_window_get_hscrollbar(sw))) {
            setupSlider(slider);
        }
        if ((slider = gtk_scrolled_window_get_vscrollbar(sw))) {
            setupSlider(slider);
        }
    }
}

}
}
