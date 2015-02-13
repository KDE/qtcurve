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

#include "combobox.h"

#include <qtcurve-utils/gtkprops.h>

namespace QtCurve {
namespace ComboBox {

/**
 * Setting appears-as-list on a non-editable combo creates a view over the
 * 'label' which is of 'base' colour. gtk_cell_view_set_background_color
 * removes this
 */
static bool
cellViewHasBgnd(GtkWidget *view)
{
    gboolean val;
    g_object_get(view, "background-set", &val, nullptr);
    return val;
}

static void
clearBgndColor(GtkWidget *widget)
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
    for (GList *child = children;child;child = child->next) {
        GtkWidget *boxChild = (GtkWidget*)child->data;
        if (GTK_IS_CELL_VIEW(boxChild) &&
            cellViewHasBgnd(boxChild)) {
            gtk_cell_view_set_background_color(GTK_CELL_VIEW(boxChild), nullptr);
        }
    }
    if (children) {
        g_list_free(children);
    }
}

static GtkWidget *comboFocus = nullptr;
static GtkWidget *comboHover = nullptr;

#if 0
static bool
appearsAsList(GtkWidget *widget)
{
    gboolean rv;
    gtk_widget_style_get(widget, "appears-as-list", &rv, nullptr);
    return rv;
}
#endif

static void
cleanup(GtkWidget *widget)
{
    if (!widget) {
        return;
    }
    GtkWidgetProps props(widget);
    if (props->comboBoxHacked) {
        props->comboBoxDestroy.disconn();
        props->comboBoxUnrealize.disconn();
        props->comboBoxStyleSet.disconn();
        props->comboBoxEnter.disconn();
        props->comboBoxLeave.disconn();
        props->comboBoxStateChange.disconn();
        props->comboBoxHacked = false;
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
enter(GtkWidget *widget, GdkEventMotion*, void *data)
{
    if (GTK_IS_EVENT_BOX(widget)) {
        GtkWidget *widget = (GtkWidget*)data;
        if (comboHover != widget) {
            comboHover = widget;
            gtk_widget_queue_draw(widget);
        }
    }
    return false;
}

static gboolean
leave(GtkWidget *widget, GdkEventMotion*, void *data)
{
    if (GTK_IS_EVENT_BOX(widget)) {
        GtkWidget *widget = (GtkWidget*)data;
        if (comboHover == widget) {
            comboHover = nullptr;
            gtk_widget_queue_draw(widget);
        }
    }
    return false;
}

static void
stateChange(GtkWidget *widget, GdkEventMotion*, void*)
{
    if (GTK_IS_CONTAINER(widget)) {
        clearBgndColor(widget);
    }
}

bool
isFocusChanged(GtkWidget *widget)
{
    if (comboFocus == widget) {
        if (!gtk_widget_has_focus(widget)) {
            comboFocus = nullptr;
            return true;
        }
    } else if (gtk_widget_has_focus(widget)) {
        comboFocus = widget;
        return true;
    }
    return false;
}

bool
isHovered(GtkWidget *widget)
{
    return widget == comboHover;
}

bool
hasFocus(GtkWidget *widget, GtkWidget *mapped)
{
    return gtk_widget_has_focus(widget) || (mapped && mapped == comboFocus);
}

void
setup(GtkWidget *frame, GtkWidget *combo)
{
    if (!combo || (!frame && hasFrame(combo))) {
        return;
    }
    GtkWidgetProps props(combo);
    if (!props->comboBoxHacked) {
        props->comboBoxHacked = true;
        clearBgndColor(combo);
        props->comboBoxStateChange.conn("state-changed", stateChange);

        if (frame) {
            GList *children = gtk_container_get_children(GTK_CONTAINER(frame));
            for (GList *child = children;child;child = child->next) {
                if (GTK_IS_EVENT_BOX(child->data)) {
                    GtkWidgetProps childProps(child->data);
                    childProps->comboBoxDestroy.conn("destroy-event", destroy);
                    childProps->comboBoxUnrealize.conn("unrealize", destroy);
                    childProps->comboBoxStyleSet.conn("style-set", styleSet);
                    childProps->comboBoxEnter.conn("enter-notify-event",
                                                   enter, combo);
                    childProps->comboBoxLeave.conn("leave-notify-event",
                                                   leave, combo);
                }
            }
            if (children) {
                g_list_free(children);
            }
        }
    }
}

bool
hasFrame(GtkWidget *widget)
{
    gboolean val;
    g_object_get(widget, "has-frame", &val, nullptr);
    return val;
}

}
}
