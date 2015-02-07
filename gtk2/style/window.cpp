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

#include "window.h"

#include <qtcurve-utils/x11qtc.h>
#include <qtcurve-utils/x11wrap.h>
#include <qtcurve-utils/gtkprops.h>
#include <qtcurve-utils/log.h>
#include <qtcurve-cairo/utils.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <common/common.h>
#include <common/config_file.h>
#include "qt_settings.h"
#include "menu.h"
#include "dbus.h"

namespace QtCurve {
namespace Window {

static GtkWidget *currentActiveWindow = nullptr;

typedef struct {
    int width;
    int height;
    int timer;
    GtkWidget *widget;
    bool locked;
} QtCWindow;

static GHashTable *table = nullptr;

static QtCWindow*
lookupHash(void *hash, bool create)
{
    QtCWindow *rv = nullptr;

    if (!table)
        table = g_hash_table_new(g_direct_hash, g_direct_equal);

    rv = (QtCWindow*)g_hash_table_lookup(table, hash);

    if (!rv && create) {
        rv = qtcNew(QtCWindow);
        rv->width = rv->height = rv->timer = 0;
        rv->widget = nullptr;
        rv->locked = false;
        g_hash_table_insert(table, hash, rv);
        rv = (QtCWindow*)g_hash_table_lookup(table, hash);
    }
    return rv;
}

static void
removeFromHash(void *hash)
{
    if (table) {
        QtCWindow *tv = lookupHash(hash, false);
        if (tv) {
            if (tv->timer) {
                g_source_remove(tv->timer);
                g_object_unref(G_OBJECT(tv->widget));
            }
            g_hash_table_remove(table, hash);
        }
    }
}

static void
cleanup(GtkWidget *widget)
{
    if (widget) {
        QTC_DEF_WIDGET_PROPS(props, widget);
        if (!(qtcIsFlatBgnd(opts.bgndAppearance)) ||
            opts.bgndImage.type != IMG_NONE) {
            removeFromHash(widget);
            qtcDisconnectFromProp(props, windowConfigure);
        }
        qtcDisconnectFromProp(props, windowDestroy);
        qtcDisconnectFromProp(props, windowStyleSet);
        if ((opts.menubarHiding & HIDE_KEYBOARD) ||
            (opts.statusbarHiding & HIDE_KEYBOARD))
            qtcDisconnectFromProp(props, windowKeyRelease);
        if ((opts.menubarHiding & HIDE_KWIN) ||
            (opts.statusbarHiding & HIDE_KWIN))
            qtcDisconnectFromProp(props, windowMap);
        if (opts.shadeMenubarOnlyWhenActive || BLEND_TITLEBAR ||
            opts.menubarHiding || opts.statusbarHiding)
            qtcDisconnectFromProp(props, windowClientEvent);
        qtcWidgetProps(props)->windowHacked = false;
    }
}

static gboolean
styleSet(GtkWidget *widget, GtkStyle*, void*)
{
    cleanup(widget);
    return false;
}

static bool toggleMenuBar(GtkWidget *widget);
static bool toggleStatusBar(GtkWidget *widget);

static gboolean
clientEvent(GtkWidget *widget, GdkEventClient *event, void*)
{
    if (gdk_x11_atom_to_xatom(event->message_type) ==
        qtc_x11_qtc_active_window) {
        if (event->data.l[0]) {
            currentActiveWindow = widget;
        } else if (currentActiveWindow == widget) {
            currentActiveWindow = 0L;
        }
        gtk_widget_queue_draw(widget);
    } else if (gdk_x11_atom_to_xatom(event->message_type) ==
               qtc_x11_qtc_titlebar_size) {
        qtcGetWindowBorderSize(true);
        GtkWidget *menubar = getMenuBar(widget, 0);

        if (menubar) {
            gtk_widget_queue_draw(menubar);
        }
    } else if (gdk_x11_atom_to_xatom(event->message_type) ==
               qtc_x11_qtc_toggle_menubar) {
        if (opts.menubarHiding & HIDE_KWIN && toggleMenuBar(widget)) {
            gtk_widget_queue_draw(widget);
        }
    } else if (gdk_x11_atom_to_xatom(event->message_type) ==
               qtc_x11_qtc_toggle_statusbar) {
        if (opts.statusbarHiding & HIDE_KWIN &&
            toggleStatusBar(widget)) {
            gtk_widget_queue_draw(widget);
        }
    }
    return false;
}

static gboolean
destroy(GtkWidget *widget, GdkEvent*, void*)
{
    cleanup(widget);
    return false;
}

static bool
sizeRequest(GtkWidget *widget)
{
    if (widget && (!(qtcIsFlatBgnd(opts.bgndAppearance)) ||
                   IMG_NONE != opts.bgndImage.type)) {
        QtcRect alloc = Widget::getAllocation(widget);
        QtcRect rect = {0, 0, 0, 0};
        if (qtcIsFlat(opts.bgndAppearance) &&
            IMG_NONE != opts.bgndImage.type) {
            EPixPos pos = (IMG_FILE == opts.bgndImage.type ?
                           opts.bgndImage.pos : PP_TR);
            if (opts.bgndImage.type == IMG_FILE) {
                qtcLoadBgndImage(&opts.bgndImage);
            }
            switch (pos) {
            case PP_TL:
                rect.width  = opts.bgndImage.width + 1;
                rect.height = opts.bgndImage.height + 1;
                break;
            case PP_TM:
            case PP_TR:
                rect.width = alloc.width;
                rect.height = (opts.bgndImage.type == IMG_FILE ?
                               opts.bgndImage.height :
                               RINGS_HEIGHT(opts.bgndImage.type)) + 1;
                break;
            case PP_LM:
            case PP_BL:
                rect.width = opts.bgndImage.width + 1;
                rect.height = alloc.height;
                break;
            case PP_CENTRED:
            case PP_BR:
            case PP_BM:
            case PP_RM:
                rect.width = alloc.width;
                rect.height = alloc.height;
                break;
            }
            if (alloc.width < rect.width) {
                rect.width = alloc.width;
            }
            if (alloc.height < rect.height) {
                rect.height = alloc.height;
            }
        } else {
            rect.width = alloc.width, rect.height = alloc.height;
        }
        gdk_window_invalidate_rect(gtk_widget_get_window(widget),
                                   (GdkRectangle*)&rect, false);
    }
    return false;
}

static gboolean
delayedUpdate(void *user_data)
{
    QtCWindow *window = (QtCWindow*)user_data;

    if (window) {
        if (window->locked) {
            window->locked = false;
            return true;
        } else {
            g_source_remove(window->timer);
            window->timer = 0;
            // otherwise, trigger update
            gdk_threads_enter();
            sizeRequest(window->widget);
            gdk_threads_leave();
            g_object_unref(G_OBJECT(window->widget));
            return false;
        }
    }
    return false;
}

static gboolean
configure(GtkWidget*, GdkEventConfigure *event, void *data)
{
    QtCWindow *window = (QtCWindow*)data;

    if (window && (event->width != window->width ||
                   event->height != window->height)) {
        window->width = event->width;
        window->height = event->height;

        // schedule delayed timeOut
        if (!window->timer) {
            g_object_ref(G_OBJECT(window->widget));
            window->timer =
                g_timeout_add(50, delayedUpdate, window);
            window->locked = false;
        } else {
            window->locked = true;
        }
    }
    return false;
}

static bool
canGetChildren(GtkWidget *widget)
{
    return (qtSettings.app != GTK_APP_GHB ||
            0 != strcmp(g_type_name(G_OBJECT_TYPE(widget)), "GhbCompositor") ||
            gtk_widget_get_realized(widget));
}

static bool
toggleMenuBar(GtkWidget *widget)
{
    GtkWidget *menuBar = getMenuBar(widget, 0);

    if (menuBar) {
        int size = 0;
        qtcSetMenuBarHidden(qtSettings.appName,
                            gtk_widget_get_visible(menuBar));
        if (gtk_widget_get_visible(menuBar)) {
            gtk_widget_hide(menuBar);
        } else {
            size = Widget::getAllocation(menuBar).height;
            gtk_widget_show(menuBar);
        }

        Menu::emitSize(menuBar, size);
        menuBarDBus(widget, size);
        return true;
    }
    return false;
}

static bool
toggleStatusBar(GtkWidget *widget)
{
    GtkWidget *statusBar = getStatusBar(widget, 0);

    if (statusBar) {
        bool state = gtk_widget_get_visible(statusBar);
        qtcSetStatusBarHidden(qtSettings.appName, state);
        if (state) {
            gtk_widget_hide(statusBar);
        } else {
            gtk_widget_show(statusBar);
        }
        statusBarDBus(widget, state);
        return true;
    }
    return false;
}

static void
setProperties(GtkWidget *w, unsigned short opacity)
{
    GtkWindow *topLevel = GTK_WINDOW(gtk_widget_get_toplevel(w));
    unsigned long prop = (qtcIsFlatBgnd(opts.bgndAppearance) ?
                          (IMG_NONE != opts.bgndImage.type ?
                           APPEARANCE_RAISED : APPEARANCE_FLAT) :
                          opts.bgndAppearance) & 0xFF;
    //GtkRcStyle *rcStyle=gtk_widget_get_modifier_style(w);
    GdkColor *bgnd = /* rcStyle ? &rcStyle->bg[GTK_STATE_NORMAL] : */
        &qtcPalette.background[ORIGINAL_SHADE];
    xcb_window_t wid =
        GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(topLevel)));

    if (opacity != 100) {
        qtcX11SetOpacity(wid, opacity);
    }
    prop |= (((toQtColor(bgnd->red) & 0xFF) << 24) |
             ((toQtColor(bgnd->green) & 0xFF) << 16) |
             ((toQtColor(bgnd->blue) & 0xFF) << 8));
    qtcX11ChangeProperty(XCB_PROP_MODE_REPLACE, wid, qtc_x11_qtc_bgnd,
                         XCB_ATOM_CARDINAL, 32, 1, &prop);
    qtcX11Flush();
}

static gboolean
keyRelease(GtkWidget *widget, GdkEventKey *event, void*)
{
    // Ensure only ctrl/alt/shift/capsLock are pressed...
    if (GDK_CONTROL_MASK & event->state && GDK_MOD1_MASK & event->state &&
        !event->is_modifier && 0 == (event->state & 0xFF00)) {
        bool toggled = false;
        if (opts.menubarHiding & HIDE_KEYBOARD &&
            (GDK_KEY_m == event->keyval || GDK_KEY_M == event->keyval)) {
            toggled = toggleMenuBar(widget);
        }
        if (opts.statusbarHiding & HIDE_KEYBOARD &&
            (GDK_KEY_s == event->keyval || GDK_KEY_S == event->keyval)) {
            toggled = toggleStatusBar(widget);
        }
        if (toggled) {
            gtk_widget_queue_draw(widget);
        }
    }
    return false;
}

static gboolean
mapWindow(GtkWidget *widget, GdkEventKey*, void*)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    setProperties(widget, qtcWidgetProps(props)->windowOpacity);

    if (opts.menubarHiding & HIDE_KWIN) {
        GtkWidget *menuBar = getMenuBar(widget, 0);

        if (menuBar) {
            int size = (gtk_widget_get_visible(menuBar) ?
                        Widget::getAllocation(menuBar).height : 0);

            Menu::emitSize(menuBar, size);
            menuBarDBus(widget, size);
        }
    }

    if (opts.statusbarHiding & HIDE_KWIN) {
        GtkWidget *statusBar = getStatusBar(widget, 0);

        if (statusBar) {
            statusBarDBus(widget, !gtk_widget_get_visible(statusBar));
        }
    }
    return false;
}

bool
isActive(GtkWidget *widget)
{
    return widget && (gtk_window_is_active(GTK_WINDOW(widget)) ||
                      currentActiveWindow == widget);
}

bool
setup(GtkWidget *widget, int opacity)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->windowHacked) {
        qtcWidgetProps(props)->windowHacked = true;
        if (!qtcIsFlatBgnd(opts.bgndAppearance) ||
            opts.bgndImage.type != IMG_NONE) {
            QtCWindow *window = lookupHash(widget, true);
            if (window) {
                QtcRect alloc = Widget::getAllocation(widget);
                qtcConnectToProp(props, windowConfigure, "configure-event",
                                 configure, window);
                window->width = alloc.width;
                window->height = alloc.height;
                window->widget = widget;
            }
        }
        qtcConnectToProp(props, windowDestroy, "destroy-event",
                         destroy, nullptr);
        qtcConnectToProp(props, windowStyleSet, "style-set",
                         styleSet, nullptr);
        if ((opts.menubarHiding & HIDE_KEYBOARD) ||
            (opts.statusbarHiding & HIDE_KEYBOARD)) {
            qtcConnectToProp(props, windowKeyRelease, "key-release-event",
                             keyRelease, nullptr);
        }
        qtcWidgetProps(props)->windowOpacity = (unsigned short)opacity;
        setProperties(widget, (unsigned short)opacity);

        if ((opts.menubarHiding & HIDE_KWIN) ||
            (opts.statusbarHiding & HIDE_KWIN) || 100 != opacity)
            qtcConnectToProp(props, windowMap, "map-event",
                             mapWindow, nullptr);
        if (opts.shadeMenubarOnlyWhenActive || BLEND_TITLEBAR ||
            opts.menubarHiding || opts.statusbarHiding)
            qtcConnectToProp(props, windowClientEvent, "client-event",
                             clientEvent, nullptr);
        return true;
    }
    return false;
}

GtkWidget*
getMenuBar(GtkWidget *parent, int level)
{
    if (level < 3 && GTK_IS_CONTAINER(parent) && canGetChildren(parent)
        /* && gtk_widget_get_realized(parent)*/) {
        GtkWidget *rv = nullptr;
        GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
        for (GList *child = children;child && !rv;child = child->next) {
            GtkWidget *boxChild = (GtkWidget*)child->data;

            if (GTK_IS_MENU_BAR(boxChild)) {
                rv = GTK_WIDGET(boxChild);
            } else if (GTK_IS_CONTAINER(boxChild)) {
                rv = getMenuBar(GTK_WIDGET(boxChild), level + 1);
            }
        }

        if (children) {
            g_list_free(children);
        }
        return rv;
    }
    return nullptr;
}

bool
setStatusBarProp(GtkWidget *w)
{
    QTC_DEF_WIDGET_PROPS(props, w);
    if (w && !qtcWidgetProps(props)->statusBarSet) {
        GtkWindow *topLevel = GTK_WINDOW(gtk_widget_get_toplevel(w));
        xcb_window_t wid =
            GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(topLevel)));

        qtcWidgetProps(props)->statusBarSet = true;
        qtcX11SetStatusBar(wid);
        return true;
    }
    return false;
}

GtkWidget*
getStatusBar(GtkWidget *parent, int level)
{
    if (level < 3 && GTK_IS_CONTAINER(parent) && canGetChildren(parent)
        /* && gtk_widget_get_realized(parent)*/) {
        GtkWidget *rv = nullptr;
        GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
        for(GList *child = children;child && !rv;child = child->next) {
            GtkWidget *boxChild = (GtkWidget*)child->data;

            if (GTK_IS_STATUSBAR(boxChild)) {
                rv=GTK_WIDGET(boxChild);
            } else if (GTK_IS_CONTAINER(boxChild)) {
                rv = getStatusBar(GTK_WIDGET(boxChild), level + 1);
            }
        }
        if (children) {
            g_list_free(children);
        }
        return rv;
    }
    return nullptr;
}

void
statusBarDBus(GtkWidget *widget, bool state)
{
    GtkWindow *topLevel = GTK_WINDOW(gtk_widget_get_toplevel(widget));
    uint32_t xid = GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(topLevel)));
    GDBus::callMethod("org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                      "statusBarState", xid, state);
}

void
menuBarDBus(GtkWidget *widget, int32_t size)
{
    GtkWindow *topLevel = GTK_WINDOW(gtk_widget_get_toplevel(widget));
    uint32_t xid = GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(topLevel)));
    GDBus::callMethod("org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                      "menuBarSize", xid, size);
}

}
}
