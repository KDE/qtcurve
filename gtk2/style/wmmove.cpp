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

static int qtcWMMoveLastX = -1;
static int qtcWMMoveLastY = -1;
static int qtcWMMoveTimer = 0;
static GtkWidget *qtcWMMoveDragWidget = NULL;
//! keep track of the last rejected button event to reject it again if passed to some parent widget
/*! this spares some time (by not processing the same event twice), and prevents some bugs */
GdkEventButton *qtcWMMoveLastRejectedEvent = NULL;

static int qtcWMMoveBtnReleaseSignalId = 0;
static int qtcWMMoveBtnReleaseHookId = 0;

static gboolean qtcWMMoveDragEnd();

static gboolean
qtcWMMoveBtnReleaseHook(GSignalInvocationHint*, unsigned, const GValue*, void*)
{
    if (qtcWMMoveDragWidget)
        qtcWMMoveDragEnd();
    return true;
}

static void
qtcWMMoveRegisterBtnReleaseHook()
{
    if (qtcWMMoveBtnReleaseSignalId == 0 && qtcWMMoveBtnReleaseHookId == 0) {
        qtcWMMoveBtnReleaseSignalId =
            g_signal_lookup("button-release-event", GTK_TYPE_WIDGET);
        if (qtcWMMoveBtnReleaseSignalId) {
            qtcWMMoveBtnReleaseHookId =
                g_signal_add_emission_hook(qtcWMMoveBtnReleaseSignalId,
                                           (GQuark)0L, qtcWMMoveBtnReleaseHook,
                                           0L, 0L);
        }
    }
}

static void
qtcWMMoveStopTimer()
{
    if (qtcWMMoveTimer)
        g_source_remove(qtcWMMoveTimer);
    qtcWMMoveTimer = 0;
}

static void
qtcWMMoveReset()
{
    qtcWMMoveLastX = -1;
    qtcWMMoveLastY = -1;
    qtcWMMoveDragWidget = NULL;
    qtcWMMoveLastRejectedEvent = NULL;
    qtcWMMoveStopTimer();
}

static void
qtcWMMoveStore(GtkWidget *widget, GdkEventButton *event)
{
    qtcWMMoveLastX = event ? event->x_root : -1;
    qtcWMMoveLastY = event ? event->y_root : -1;
    qtcWMMoveDragWidget = widget;
}

static void
qtcWMMoveTrigger(GtkWidget *w, int x, int y)
{
    GtkWindow *topLevel = GTK_WINDOW(gtk_widget_get_toplevel(w));
    xcb_window_t wid =
        GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(topLevel)));
    qtcX11MoveTrigger(wid, x, y);
    qtcWMMoveDragEnd();
}

static gboolean qtcWMMoveWithinWidget(GtkWidget *widget, GdkEventButton *event)
{
    // get top level widget
    GtkWidget *topLevel=gtk_widget_get_toplevel(widget);;
    GdkWindow *window = topLevel ? gtk_widget_get_window(topLevel) : NULL;

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
            QtcRect widgetAlloc = qtcWidgetGetAllocation(widget);
            allocation = qtcTabGetTabbarRect(GTK_NOTEBOOK(widget));
            allocation.x += wx - widgetAlloc.x;
            allocation.y += wy - widgetAlloc.y;
        } else {
            allocation = qtcWidgetGetAllocation(widget);
            allocation.x = wx;
            allocation.y = wy;
        }

        return allocation.x<=event->x_root && allocation.y<=event->y_root &&
            (allocation.x+allocation.width)>event->x_root && (allocation.y+allocation.height)>event->y_root;
    }
    return true;
}

static gboolean qtcWMMoveIsBlackListed(GObject *object)
{
    static const char *widgets[] = {
        "GtkPizza", "GladeDesignLayout", "MetaFrames", "SPHRuler", "SPVRuler", 0
    };

    for (int i = 0;widgets[i];i++) {
        if (QtCurve::objectIsA(object, widgets[i])) {
            return true;
        }
    }
    return false;
}

static gboolean
qtcWMMoveChildrenUseEvent(GtkWidget *widget, GdkEventButton *event,
                          gboolean inNoteBook)
{
    // accept, by default
    bool usable = true;

    // get children and check
    GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
    for (GList *child = children;child && usable;child = g_list_next(child)) {
        // cast child to GtkWidget
        if (GTK_IS_WIDGET(child->data)) {
            GtkWidget *childWidget = GTK_WIDGET(child->data);
            GdkWindow *window = NULL;

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

            if(!(event && qtcWMMoveWithinWidget(childWidget, event)))
                continue;

            // check special cases for which grab should not be enabled
            if((qtcWMMoveIsBlackListed(G_OBJECT(childWidget))) ||
               (GTK_IS_NOTEBOOK(widget) && qtcTabIsLabel(GTK_NOTEBOOK(widget),
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
                usable = qtcWMMoveChildrenUseEvent(childWidget, event,
                                                   inNoteBook);
            }
        }
    }
    if (children) {
        g_list_free(children);
    }
    return usable;
}

static gboolean qtcWMMoveUseEvent(GtkWidget *widget, GdkEventButton *event)
{
    if(qtcWMMoveLastRejectedEvent && qtcWMMoveLastRejectedEvent==event)
        return false;

    if(!GTK_IS_CONTAINER(widget))
        return true;

    // if widget is a notebook, accept if there is no hovered tab
    if(GTK_IS_NOTEBOOK(widget))
        return !qtcTabHasVisibleArrows(GTK_NOTEBOOK(widget)) && -1==qtcTabCurrentHoveredIndex(widget) && qtcWMMoveChildrenUseEvent(widget, event, false);
    else
        return qtcWMMoveChildrenUseEvent(widget, event, false);
}

static gboolean
qtcWWMoveStartDelayedDrag(void*)
{
    if (qtcWMMoveDragWidget) {
        gdk_threads_enter();
        qtcWMMoveTrigger(qtcWMMoveDragWidget, qtcWMMoveLastX, qtcWMMoveLastY);
        gdk_threads_leave();
    }
    return false;
}

static gboolean
qtcWMMoveIsWindowDragWidget(GtkWidget *widget, GdkEventButton *event)
{
    if(QtCurve::opts.windowDrag && (!event || (qtcWMMoveWithinWidget(widget, event) && qtcWMMoveUseEvent(widget, event))))
    {
        qtcWMMoveStore(widget, event);
        // Start timer
        qtcWMMoveStopTimer();
        qtcWMMoveTimer=g_timeout_add(QtCurve::qtSettings.startDragTime,
                                     (GSourceFunc)qtcWWMoveStartDelayedDrag, NULL);
        return true;
    }
    qtcWMMoveLastRejectedEvent=event;
    return false;
}

static gboolean
qtcWMMoveButtonPress(GtkWidget *widget, GdkEventButton *event, void*)
{
    if (GDK_BUTTON_PRESS == event->type && 1 == event->button &&
        qtcWMMoveIsWindowDragWidget(widget, event)) {
        qtcWMMoveDragWidget = widget;
        return true;
    }
    return false;
}

static gboolean qtcWMMoveDragEnd()
{
    if (qtcWMMoveDragWidget) {
        //gtk_grab_remove(widget);
        gdk_pointer_ungrab(CurrentTime);
        qtcWMMoveReset();
        return true;
    }

    return false;
}

static void qtcWMMoveCleanup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (qtcWidgetProps(props)->wmMoveHacked) {
        if (widget == qtcWMMoveDragWidget)
            qtcWMMoveReset();
        qtcDisconnectFromProp(props, wmMoveDestroy);
        qtcDisconnectFromProp(props, wmMoveStyleSet);
        qtcDisconnectFromProp(props, wmMoveMotion);
        qtcDisconnectFromProp(props, wmMoveLeave);
        qtcDisconnectFromProp(props, wmMoveButtonPress);
        qtcWidgetProps(props)->wmMoveHacked = false;
    }
}

static gboolean
qtcWMMoveStyleSet(GtkWidget *widget, GtkStyle*, void*)
{
    qtcWMMoveCleanup(widget);
    return false;
}

static gboolean
qtcWMMoveDestroy(GtkWidget *widget, GdkEvent*, void*)
{
    qtcWMMoveCleanup(widget);
    return false;
}

static gboolean
qtcWMMoveMotion(GtkWidget *widget, GdkEventMotion *event, void*)
{
    if (qtcWMMoveDragWidget == widget) {
        // check displacement with respect to drag start
        const int distance = (qtcAbs(qtcWMMoveLastX - event->x_root) +
                              qtcAbs(qtcWMMoveLastY - event->y_root));

        if (distance > 0)
            qtcWMMoveStopTimer();

        /* if (distance < QtCurve::qtSettings.startDragDist) */
        /*     return false; */
        qtcWMMoveTrigger(widget, event->x_root, event->y_root);
        return true;
    }
    return false;
}

static gboolean
qtcWMMoveLeave(GtkWidget*, GdkEventMotion*, void*)
{
    return qtcWMMoveDragEnd();
}

void qtcWMMoveSetup(GtkWidget *widget)
{
    QTC_RET_IF_FAIL(widget);
    GtkWidget *parent = NULL;

    if (GTK_IS_WINDOW(widget) &&
        !gtk_window_get_decorated(GTK_WINDOW(widget))) {
        return;
    }

    if (GTK_IS_EVENT_BOX(widget) &&
        gtk_event_box_get_above_child(GTK_EVENT_BOX(widget)))
        return;

    parent = gtk_widget_get_parent(widget);

    // widgets used in tabs also must be ignored (happens, unfortunately)
    if (GTK_IS_NOTEBOOK(parent) && qtcTabIsLabel(GTK_NOTEBOOK(parent), widget))
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

    QTC_DEF_WIDGET_PROPS(props, widget);
    if (!QtCurve::isFakeGtk() && !qtcWidgetProps(props)->wmMoveHacked) {
        qtcWidgetProps(props)->wmMoveHacked = true;
        gtk_widget_add_events(widget, GDK_BUTTON_RELEASE_MASK |
                              GDK_BUTTON_PRESS_MASK | GDK_LEAVE_NOTIFY_MASK |
                              GDK_BUTTON1_MOTION_MASK);
        qtcWMMoveRegisterBtnReleaseHook();
        qtcConnectToProp(props, wmMoveDestroy, "destroy-event",
                         qtcWMMoveDestroy, NULL);
        qtcConnectToProp(props, wmMoveStyleSet, "style-set",
                         qtcWMMoveStyleSet, NULL);
        qtcConnectToProp(props, wmMoveMotion, "motion-notify-event",
                         qtcWMMoveMotion, NULL);
        qtcConnectToProp(props, wmMoveLeave, "leave-notify-event",
                         qtcWMMoveLeave, NULL);
        qtcConnectToProp(props, wmMoveButtonPress, "button-press-event",
                         qtcWMMoveButtonPress, NULL);
    }
}
