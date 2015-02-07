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

#include "menu.h"

#include <qtcurve-utils/gtkprops.h>
#include <qtcurve-utils/x11qtc.h>
#include <qtcurve-cairo/utils.h>

#include <gdk/gdkx.h>
#include <common/common.h>

namespace QtCurve {
namespace Menu {

#define EXTEND_MENUBAR_ITEM_HACK

#ifdef EXTEND_MENUBAR_ITEM_HACK
static bool
isSelectable(GtkWidget *menu)
{
    return !((!gtk_bin_get_child(GTK_BIN(menu)) &&
              G_OBJECT_TYPE(menu) == GTK_TYPE_MENU_ITEM) ||
             GTK_IS_SEPARATOR_MENU_ITEM(menu) ||
             !gtk_widget_is_sensitive(menu) ||
             !gtk_widget_get_visible(menu));
}

static gboolean
shellButtonPress(GtkWidget *widget, GdkEventButton *event, void*)
{
    if (GTK_IS_MENU_BAR(widget)) {
        // QtCurve's menubars have a 2 pixel border ->
        // but want the left/top to be 'active'...
        int nx, ny;
        gdk_window_get_origin(gtk_widget_get_window(widget), &nx, &ny);
        if ((event->x_root - nx) <= 2.0 || (event->y_root - ny) <= 2.0) {
            if ((event->x_root - nx) <= 2.0) {
                event->x_root += 2.0;
            }
            if ((event->y_root - ny) <= 2.0) {
                event->y_root += 2.0;
            }

            GtkMenuShell *menuShell = GTK_MENU_SHELL(widget);
            GList *children =
                gtk_container_get_children(GTK_CONTAINER(menuShell));
            bool rv = false;
            for (GList *child = children;child;child = child->next) {
                GtkWidget *item = (GtkWidget*)child->data;
                QtcRect alloc = Widget::getAllocation(item);
                int cx = alloc.x + nx;
                int cy = alloc.y + ny;
                int cw = alloc.width;
                int ch = alloc.height;
                if (cx <= event->x_root && cy <= event->y_root &&
                    (cx + cw) > event->x_root && (cy + ch) > event->y_root) {
                    if (isSelectable(item)) {
                        if (event->type == GDK_BUTTON_PRESS) {
                            if (item != menuShell->active_menu_item) {
                                menuShell->active = false;
                                gtk_menu_shell_select_item(menuShell, item);
                                menuShell->active = true;
                            } else {
                                menuShell->active = true;
                                gtk_menu_shell_deselect(menuShell);
                                menuShell->active = false;
                            }
                        }
                        rv = true;
                    }
                    break;
                }
            }
            if (children) {
                g_list_free(children);
            }
            return rv;
        }
    }
    return false;
}
#endif

static void
shellCleanup(GtkWidget *widget)
{
    if (GTK_IS_MENU_BAR(widget)) {
        QTC_DEF_WIDGET_PROPS(props, widget);
        qtcDisconnectFromProp(props, menuShellMotion);
        qtcDisconnectFromProp(props, menuShellLeave);
        qtcDisconnectFromProp(props, menuShellDestroy);
        qtcDisconnectFromProp(props, menuShellStyleSet);
#ifdef EXTEND_MENUBAR_ITEM_HACK
        qtcDisconnectFromProp(props, menuShellButtonPress);
        qtcDisconnectFromProp(props, menuShellButtonRelease);
#endif
        qtcWidgetProps(props)->menuShellHacked = true;
    }
}

static gboolean
shellStyleSet(GtkWidget *widget, GtkStyle*, void*)
{
    shellCleanup(widget);
    return false;
}

static gboolean
shellDestroy(GtkWidget *widget, GdkEvent*, void*)
{
    shellCleanup(widget);
    return false;
}

static gboolean
shellMotion(GtkWidget *widget, GdkEventMotion*, void*)
{
    if (GTK_IS_MENU_SHELL(widget)) {
        int pointer_x, pointer_y;
        GdkModifierType pointer_mask;

        gdk_window_get_pointer(gtk_widget_get_window(widget), &pointer_x,
                               &pointer_y, &pointer_mask);

        if (GTK_IS_CONTAINER(widget)) {
            GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
            for (GList *child = children;child;child = g_list_next(child)) {
                if ((child->data) && GTK_IS_WIDGET(child->data) &&
                    (gtk_widget_get_state(GTK_WIDGET(child->data)) !=
                     GTK_STATE_INSENSITIVE)) {
                    QtcRect alloc =
                        Widget::getAllocation(GTK_WIDGET(child->data));

                    if ((pointer_x >= alloc.x) && (pointer_y >= alloc.y) &&
                        (pointer_x < (alloc.x + alloc.width)) &&
                        (pointer_y < (alloc.y + alloc.height))) {
                        gtk_widget_set_state(GTK_WIDGET(child->data),
                                             GTK_STATE_PRELIGHT);
                    } else {
                        gtk_widget_set_state(GTK_WIDGET(child->data),
                                             GTK_STATE_NORMAL);
                    }
                }
            }
            if (children) {
                g_list_free(children);
            }
        }
    }

    return false;
}

static gboolean
shellLeave(GtkWidget *widget, GdkEventCrossing*, void*)
{
    if (GTK_IS_MENU_SHELL(widget) && GTK_IS_CONTAINER(widget)) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
        for (GList *child = children;child;child = g_list_next(child)) {
            if ((child->data) && GTK_IS_MENU_ITEM(child->data) &&
                (gtk_widget_get_state(GTK_WIDGET(child->data)) !=
                 GTK_STATE_INSENSITIVE)) {
                GtkWidget *submenu =
                    gtk_menu_item_get_submenu(GTK_MENU_ITEM(child->data));
                GtkWidget *topLevel =
                    submenu ? gtk_widget_get_toplevel(submenu) : NULL;

                if (submenu &&
                    ((!GTK_IS_MENU(submenu)) ||
                     (!(gtk_widget_get_realized(submenu) &&
                        gtk_widget_get_visible(submenu) &&
                        gtk_widget_get_realized(topLevel) &&
                        gtk_widget_get_visible(topLevel))))) {
                    gtk_widget_set_state(GTK_WIDGET(child->data),
                                         GTK_STATE_NORMAL);
                }
            }
        }
        if (children) {
            g_list_free(children);
        }
    }
    return false;
}

void
shellSetup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (GTK_IS_MENU_BAR(widget) && !qtcWidgetProps(props)->menuShellHacked) {
        qtcWidgetProps(props)->menuShellHacked = true;
        qtcConnectToProp(props, menuShellMotion, "motion-notify-event",
                         shellMotion, NULL);
        qtcConnectToProp(props, menuShellLeave, "leave-notify-event",
                         shellLeave, NULL);
        qtcConnectToProp(props, menuShellDestroy, "destroy-event",
                         shellDestroy, NULL);
        qtcConnectToProp(props, menuShellStyleSet, "style-set",
                         shellStyleSet, NULL);
#ifdef EXTEND_MENUBAR_ITEM_HACK
        qtcConnectToProp(props, menuShellButtonPress, "button-press-event",
                         shellButtonPress, NULL);
        qtcConnectToProp(props, menuShellButtonRelease, "button-release-event",
                         shellButtonPress, NULL);
#endif
    }
}

bool
emitSize(GtkWidget *w, unsigned size)
{
    if (w) {
        QTC_DEF_WIDGET_PROPS(props, w);
        unsigned oldSize = qtcWidgetProps(props)->menuBarSize;

        if (oldSize != size) {
            GtkWidget *topLevel = gtk_widget_get_toplevel(w);
            xcb_window_t wid =
                GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(topLevel)));

            if (size == 0xFFFF) {
                size = 0;
            }
            qtcWidgetProps(props)->menuBarSize = size;
            qtcX11SetMenubarSize(wid, size);
            return true;
        }
    }
    return false;
}

}
}
