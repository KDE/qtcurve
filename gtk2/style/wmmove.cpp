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

#include "wmmove.h"

#include <qtcurve-utils/gtkprops.h>
#include <qtcurve-utils/x11wmmove.h>

#include <gdk/gdkx.h>
#include "helpers.h"
#include "qt_settings.h"
#include "tab.h"

namespace QtCurve {
namespace WMMove {

static int lastX = -1;
static int lastY = -1;
static int timer = 0;
static GtkWidget *dragWidget = nullptr;
//! keep track of the last rejected button event to reject it again if passed to some parent widget
/*! this spares some time (by not processing the same event twice), and prevents some bugs */
GdkEventButton *lastRejectedEvent = nullptr;

static int btnReleaseSignalId = 0;
static int btnReleaseHookId = 0;

static bool dragEnd();

static gboolean
btnReleaseHook(GSignalInvocationHint*, unsigned, const GValue*, void*)
{
    if (dragWidget)
        dragEnd();
    return true;
}

static void
registerBtnReleaseHook()
{
    if (btnReleaseSignalId == 0 && btnReleaseHookId == 0) {
        btnReleaseSignalId =
            g_signal_lookup("button-release-event", GTK_TYPE_WIDGET);
        if (btnReleaseSignalId) {
            btnReleaseHookId =
                g_signal_add_emission_hook(btnReleaseSignalId,
                                           (GQuark)0, btnReleaseHook,
                                           nullptr, nullptr);
        }
    }
}

static void
stopTimer()
{
    if (timer)
        g_source_remove(timer);
    timer = 0;
}

static void
reset()
{
    lastX = -1;
    lastY = -1;
    dragWidget = nullptr;
    lastRejectedEvent = nullptr;
    stopTimer();
}

static void
store(GtkWidget *widget, GdkEventButton *event)
{
    lastX = event ? event->x_root : -1;
    lastY = event ? event->y_root : -1;
    dragWidget = widget;
}

static void
trigger(GtkWidget *w, int x, int y)
{
    GtkWindow *topLevel = GTK_WINDOW(gtk_widget_get_toplevel(w));
    xcb_window_t wid =
        GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(topLevel)));
    qtcX11MoveTrigger(wid, x, y);
    dragEnd();
}

static bool
withinWidget(GtkWidget *widget, GdkEventButton *event)
{
    // get top level widget
    GtkWidget *topLevel=gtk_widget_get_toplevel(widget);;
    GdkWindow *window = topLevel ? gtk_widget_get_window(topLevel) : nullptr;

    if (window) {
        QtcRect allocation;
        int           wx=0, wy=0, nx=0, ny=0;

        // translate widget position to topLevel
        gtk_widget_translate_coordinates(widget, topLevel, wx, wy, &wx, &wy);

        // translate to absolute coordinates
        gdk_window_get_origin(window, &nx, &ny);
        wx += nx;
        wy += ny;

        // get widget size.
        // for notebooks, only consider the tabbar rect
        if (GTK_IS_NOTEBOOK(widget)) {
            QtcRect widgetAlloc = Widget::getAllocation(widget);
            allocation = Tab::getTabbarRect(GTK_NOTEBOOK(widget));
            allocation.x += wx - widgetAlloc.x;
            allocation.y += wy - widgetAlloc.y;
        } else {
            allocation = Widget::getAllocation(widget);
            allocation.x = wx;
            allocation.y = wy;
        }

        return allocation.x<=event->x_root && allocation.y<=event->y_root &&
            (allocation.x+allocation.width)>event->x_root && (allocation.y+allocation.height)>event->y_root;
    }
    return true;
}

static bool
isBlackListed(GObject *object)
{
    static const char *widgets[] = {
        "GtkPizza", "GladeDesignLayout", "MetaFrames", "SPHRuler", "SPVRuler", 0
    };

    for (int i = 0;widgets[i];i++) {
        if (objectIsA(object, widgets[i])) {
            return true;
        }
    }
    return false;
}

static bool
childrenUseEvent(GtkWidget *widget, GdkEventButton *event, bool inNoteBook)
{
    // accept, by default
    bool usable = true;

    // get children and check
    GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
    for (GList *child = children;child && usable;child = g_list_next(child)) {
        // cast child to GtkWidget
        if (GTK_IS_WIDGET(child->data)) {
            GtkWidget *childWidget = GTK_WIDGET(child->data);
            GdkWindow *window = nullptr;

            // check widget state and type
            if (gtk_widget_get_state(childWidget) == GTK_STATE_PRELIGHT) {
                // if widget is prelight, we don't need to check where event
                // happen, any prelight widget indicate we can't do a move
                usable = false;
                continue;
            }

            window = gtk_widget_get_window(childWidget);
            if (!(window && gdk_window_is_visible(window)))
                continue;

            if (GTK_IS_NOTEBOOK(childWidget))
                inNoteBook = true;

            if(!(event && withinWidget(childWidget, event)))
                continue;

            // check special cases for which grab should not be enabled
            if((isBlackListed(G_OBJECT(childWidget))) ||
               (GTK_IS_NOTEBOOK(widget) && Tab::isLabel(GTK_NOTEBOOK(widget),
                                                         childWidget)) ||
               (GTK_IS_BUTTON(childWidget) &&
                gtk_widget_get_state(childWidget) != GTK_STATE_INSENSITIVE) ||
               (gtk_widget_get_events(childWidget) &
                (GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK)) ||
               (GTK_IS_MENU_ITEM(childWidget)) ||
               (GTK_IS_SCROLLED_WINDOW(childWidget) &&
                (!inNoteBook || gtk_widget_is_focus(childWidget)))) {
                usable = false;
            }

            // if child is a container and event has been accepted so far,
            // also check it, recursively
            if (usable && GTK_IS_CONTAINER(childWidget)) {
                usable = childrenUseEvent(childWidget, event, inNoteBook);
            }
        }
    }
    if (children) {
        g_list_free(children);
    }
    return usable;
}

static bool
useEvent(GtkWidget *widget, GdkEventButton *event)
{
    if(lastRejectedEvent && lastRejectedEvent==event)
        return false;

    if(!GTK_IS_CONTAINER(widget))
        return true;

    // if widget is a notebook, accept if there is no hovered tab
    if (GTK_IS_NOTEBOOK(widget)) {
        return (!Tab::hasVisibleArrows(GTK_NOTEBOOK(widget)) &&
                Tab::currentHoveredIndex(widget) == -1 &&
                childrenUseEvent(widget, event, false));
    } else {
        return childrenUseEvent(widget, event, false);
    }
}

static gboolean
startDelayedDrag(void*)
{
    if (dragWidget) {
        gdk_threads_enter();
        trigger(dragWidget, lastX, lastY);
        gdk_threads_leave();
    }
    return false;
}

static bool
isWindowDragWidget(GtkWidget *widget, GdkEventButton *event)
{
    if (opts.windowDrag && (!event || (withinWidget(widget, event) &&
                                       useEvent(widget, event)))) {
        store(widget, event);
        // Start timer
        stopTimer();
        timer = g_timeout_add(qtSettings.startDragTime,
                              (GSourceFunc)startDelayedDrag, nullptr);
        return true;
    }
    lastRejectedEvent=event;
    return false;
}

static gboolean
buttonPress(GtkWidget *widget, GdkEventButton *event, void*)
{
    if (GDK_BUTTON_PRESS == event->type && 1 == event->button &&
        isWindowDragWidget(widget, event)) {
        dragWidget = widget;
        return true;
    }
    return false;
}

static bool
dragEnd()
{
    if (dragWidget) {
        //gtk_grab_remove(widget);
        gdk_pointer_ungrab(CurrentTime);
        reset();
        return true;
    }

    return false;
}

static void cleanup(GtkWidget *widget)
{
    GtkWidgetProps props(widget);
    if (props->wmMoveHacked) {
        if (widget == dragWidget)
            reset();
        props->wmMoveDestroy.disconn();
        props->wmMoveStyleSet.disconn();
        props->wmMoveMotion.disconn();
        props->wmMoveLeave.disconn();
        props->wmMoveButtonPress.disconn();
        props->wmMoveHacked = false;
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
motion(GtkWidget *widget, GdkEventMotion *event, void*)
{
    if (dragWidget == widget) {
        // check displacement with respect to drag start
        const int distance = (qtcAbs(lastX - event->x_root) +
                              qtcAbs(lastY - event->y_root));

        if (distance > 0)
            stopTimer();

        /* if (distance < qtSettings.startDragDist) */
        /*     return false; */
        trigger(widget, event->x_root, event->y_root);
        return true;
    }
    return false;
}

static gboolean
leave(GtkWidget*, GdkEventMotion*, void*)
{
    return dragEnd();
}

void
setup(GtkWidget *widget)
{
    QTC_RET_IF_FAIL(widget);
    GtkWidget *parent = nullptr;

    if (GTK_IS_WINDOW(widget) &&
        !gtk_window_get_decorated(GTK_WINDOW(widget))) {
        return;
    }

    if (GTK_IS_EVENT_BOX(widget) &&
        gtk_event_box_get_above_child(GTK_EVENT_BOX(widget)))
        return;

    parent = gtk_widget_get_parent(widget);

    // widgets used in tabs also must be ignored (happens, unfortunately)
    if (GTK_IS_NOTEBOOK(parent) && Tab::isLabel(GTK_NOTEBOOK(parent), widget))
        return;

    /*
      check event mask (for now we only need to do that for GtkWindow)
      The idea is that if the window has been set to receive button_press
      and button_release events (which is not done by default), it likely
      means that it does something with such events, in which case we should
      not use them for grabbing
    */
    if (0 == strcmp(g_type_name(G_OBJECT_TYPE(widget)), "GtkWindow") &&
       (gtk_widget_get_events(widget) &
        (GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK)))
        return;

    GtkWidgetProps props(widget);
    if (!isFakeGtk() && !props->wmMoveHacked) {
        props->wmMoveHacked = true;
        gtk_widget_add_events(widget, GDK_BUTTON_RELEASE_MASK |
                              GDK_BUTTON_PRESS_MASK | GDK_LEAVE_NOTIFY_MASK |
                              GDK_BUTTON1_MOTION_MASK);
        registerBtnReleaseHook();
        props->wmMoveDestroy.conn("destroy-event", destroy);
        props->wmMoveStyleSet.conn("style-set", styleSet);
        props->wmMoveMotion.conn("motion-notify-event", motion);
        props->wmMoveLeave.conn("leave-notify-event", leave);
        props->wmMoveButtonPress.conn("button-press-event", buttonPress);
    }
}

}
}
