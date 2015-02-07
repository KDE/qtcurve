/*****************************************************************************
 *   Copyright 2003 - 2011 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#include "shadowhelper.h"

#include <gdk/gdkx.h>
#include <common/common.h>
#include "qt_settings.h"
#include <qtcurve-utils/x11shadow.h>
#include <qtcurve-utils/gtkprops.h>

namespace QtCurve {
namespace Shadow {

static unsigned realizeSignalId = 0;
static unsigned long realizeHookId = 0;

static void
installX11Shadows(GtkWidget* widget)
{
    if (qtSettings.debug == DEBUG_ALL)
        printf(DEBUG_PREFIX "%s\n", __FUNCTION__);
    GdkWindow *window = gtk_widget_get_window(widget);
    qtcX11ShadowInstall(GDK_WINDOW_XID(window));
}

static bool
acceptWidget(GtkWidget* widget)
{
    if (qtSettings.debug == DEBUG_ALL)
        printf(DEBUG_PREFIX "%s %p\n", __FUNCTION__, widget);

    if (widget && GTK_IS_WINDOW(widget)) {
        if (qtSettings.app == GTK_APP_OPEN_OFFICE) {
            return true;
        } else {
            GdkWindowTypeHint hint =
                gtk_window_get_type_hint(GTK_WINDOW(widget));
            if (qtSettings.debug == DEBUG_ALL)
                printf(DEBUG_PREFIX "%s %d\n", __FUNCTION__, (int)hint);
            return (hint == GDK_WINDOW_TYPE_HINT_MENU ||
                    hint == GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU ||
                    hint == GDK_WINDOW_TYPE_HINT_POPUP_MENU ||
                    hint == GDK_WINDOW_TYPE_HINT_COMBO ||
                    hint == GDK_WINDOW_TYPE_HINT_TOOLTIP /* || */
                    /* (hint == GDK_WINDOW_TYPE_HINT_UTILITY && */
                    /*  !qtcWidgetGetParent(widget) && isMozilla()) */
                    /* // Firefox URL combo */);
        }
    }
    return false;
}

static gboolean
destroy(GtkWidget *widget, void*)
{
    if (qtSettings.debug == DEBUG_ALL)
        printf(DEBUG_PREFIX "%s %p\n", __FUNCTION__, widget);

    QTC_DEF_WIDGET_PROPS(props, widget);
    if (qtcWidgetProps(props)->shadowSet) {
        qtcDisconnectFromProp(props, shadowDestroy);
        qtcWidgetProps(props)->shadowSet = false;
    }
    return false;
}

static bool
registerWidget(GtkWidget* widget)
{
    if (qtSettings.debug == DEBUG_ALL)
        printf(DEBUG_PREFIX "%s %p\n", __FUNCTION__, widget);
    // check widget
    if (!(widget && GTK_IS_WINDOW(widget)))
        return false;

    QTC_DEF_WIDGET_PROPS(props, widget);
    // make sure that widget is not already registered
    if (qtcWidgetProps(props)->shadowSet)
        return false;

    // check if window is accepted
    if (!acceptWidget(widget))
        return false;

    // try install shadows
    installX11Shadows(widget);

    qtcWidgetProps(props)->shadowSet = true;
    qtcConnectToProp(props, shadowDestroy, "destroy", destroy, nullptr);
    return true;
}

static gboolean
realizeHook(GSignalInvocationHint*, unsigned, const GValue *params, void*)
{
    GtkWidget *widget = GTK_WIDGET(g_value_get_object(params));

    if (qtSettings.debug == DEBUG_ALL)
        printf(DEBUG_PREFIX "%s %p\n", __FUNCTION__, widget);

    if (!GTK_IS_WIDGET(widget))
        return false;
    registerWidget(widget);
    return true;
}

void initialize()
{
    if (qtSettings.debug == DEBUG_ALL)
        printf(DEBUG_PREFIX "%s %d\n", __FUNCTION__, qtSettings.app);
    if (!realizeSignalId) {
        realizeSignalId = g_signal_lookup("realize", GTK_TYPE_WIDGET);
        if (realizeSignalId) {
            realizeHookId = g_signal_add_emission_hook(
                realizeSignalId, (GQuark)0, realizeHook,
                0, 0L);
        }
    }
}

}
}
