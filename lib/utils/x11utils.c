/*****************************************************************************
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

// TODO multi screen?

#include "config.h"
#include "x11utils.h"

QTC_EXPORT xcb_atom_t qtc_x11_qtc_menubar_size;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_statusbar;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_titlebar_size;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_active_window;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_toggle_menubar;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_toggle_statusbar;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_opacity;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_bgnd;

#ifdef QTC_ENABLE_X11

#include "x11shadow_p.h"
#include "x11wrap.h"
#include "log.h"
#include "x11utils_p.h"
#include <X11/Xlib-xcb.h>
// #include <X11/Xutil.h>
// #include <X11/extensions/Xrender.h>

void *qtc_disp = NULL;
xcb_connection_t *qtc_xcb_conn = NULL;
int qtc_default_screen_no = -1;
xcb_window_t qtc_root_window = {0};
xcb_screen_t *qtc_default_screen = NULL;
static char wm_cm_s_atom_name[100] = "_NET_WM_CM_S";

xcb_atom_t qtc_x11_net_wm_moveresize;
xcb_atom_t qtc_x11_net_wm_cm_s_default;
xcb_atom_t qtc_x11_kde_net_wm_shadow;
xcb_atom_t qtc_x11_kde_net_wm_blur_behind_region;
static xcb_atom_t qtc_x11_xembed_info;

static const struct {
    xcb_atom_t *atom;
    const char *name;
} qtc_x11_atoms[] = {
    {&qtc_x11_net_wm_moveresize, "_NET_WM_MOVERESIZE"},
    {&qtc_x11_net_wm_cm_s_default, wm_cm_s_atom_name},
    {&qtc_x11_kde_net_wm_shadow, "_KDE_NET_WM_SHADOW"},
    {&qtc_x11_kde_net_wm_blur_behind_region, "_KDE_NET_WM_BLUR_BEHIND_REGION"},
    {&qtc_x11_qtc_menubar_size, "_QTCURVE_MENUBAR_SIZE_"},
    {&qtc_x11_qtc_statusbar, "_QTCURVE_STATUSBAR_"},
    {&qtc_x11_qtc_titlebar_size, "_QTCURVE_TITLEBAR_SIZE_"},
    {&qtc_x11_qtc_active_window, "_QTCURVE_ACTIVE_WINDOW_"},
    {&qtc_x11_qtc_toggle_menubar, "_QTCURVE_TOGGLE_MENUBAR_"},
    {&qtc_x11_qtc_toggle_statusbar, "_QTCURVE_TOGGLE_STATUSBAR_"},
    {&qtc_x11_qtc_opacity, "_QTCURVE_OPACITY_"},
    {&qtc_x11_qtc_bgnd, "_QTCURVE_BGND_"},
    {&qtc_x11_xembed_info, "_XEMBED_INFO"}
};
#define QTC_X11_ATOM_N (sizeof(qtc_x11_atoms) / sizeof(qtc_x11_atoms[0]))

QTC_EXPORT bool
qtcX11Enabled()
{
    return qtc_xcb_conn != NULL;
}

QTC_EXPORT xcb_window_t
qtcX11RootWindow(int scrn_no)
{
    if (scrn_no < 0 || scrn_no == qtc_default_screen_no) {
        return qtc_root_window;
    }
    return qtcX11GetScreen(scrn_no)->root;
}

QTC_EXPORT int
qtcX11DefaultScreenNo()
{
    return qtc_default_screen_no;
}

QTC_EXPORT xcb_screen_t*
qtcX11DefaultScreen()
{
    return qtc_default_screen;
}

static xcb_screen_t*
screen_of_display(xcb_connection_t *c, int screen)
{
    xcb_screen_iterator_t iter;

    iter = xcb_setup_roots_iterator(xcb_get_setup(c));
    for (;iter.rem;--screen, xcb_screen_next(&iter)) {
        if (screen == 0) {
            return iter.data;
        }
    }
    return NULL;
}

static void
qtcX11AtomsInit()
{
    xcb_connection_t *conn = qtc_xcb_conn;
    xcb_intern_atom_cookie_t cookies[QTC_X11_ATOM_N];
    for (size_t i = 0;i < QTC_X11_ATOM_N;i++) {
        cookies[i] = xcb_intern_atom(conn, 0, strlen(qtc_x11_atoms[i].name),
                                     qtc_x11_atoms[i].name);
    }
    for (size_t i = 0;i < QTC_X11_ATOM_N;i++) {
        xcb_intern_atom_reply_t *r =
            xcb_intern_atom_reply(conn, cookies[i], NULL);
        if (qtcLikely(r)) {
            *qtc_x11_atoms[i].atom = r->atom;
            free(r);
        } else {
            *qtc_x11_atoms[i].atom = 0;
        }
    }
}

QTC_EXPORT xcb_screen_t*
qtcX11GetScreen(int screen_no)
{
    QTC_RET_IF_FAIL(qtc_xcb_conn, NULL);
    if (screen_no == -1 || screen_no == qtc_default_screen_no) {
        return qtc_default_screen;
    }
    return screen_of_display(qtc_xcb_conn, screen_no);
}

QTC_EXPORT void
qtcX11InitXcb(xcb_connection_t *conn, int screen_no)
{
    QTC_RET_IF_FAIL(!qtc_xcb_conn && conn);
    if (screen_no < 0) {
        screen_no = 0;
    }
    qtc_xcb_conn = conn;
    qtc_default_screen_no = screen_no;
    qtc_default_screen = screen_of_display(conn, screen_no);
    if (qtc_default_screen) {
        qtc_root_window = qtc_default_screen->root;
    }
    const size_t base_len = strlen("_NET_WM_CM_S");
    sprintf(wm_cm_s_atom_name + base_len, "%d", screen_no);
    qtcX11AtomsInit();
    qtcX11ShadowInit();
}

QTC_EXPORT void
qtcX11InitXlib(void *disp)
{
    QTC_RET_IF_FAIL(!qtc_xcb_conn && disp);
    qtc_disp = disp;
    qtcX11InitXcb(XGetXCBConnection(disp), DefaultScreen(disp));
}

QTC_EXPORT xcb_connection_t*
qtcX11GetConn()
{
    return qtc_xcb_conn;
}

QTC_EXPORT void*
qtcX11GetDisp()
{
    return qtc_disp;
}

QTC_EXPORT void
qtcX11MapRaised(xcb_window_t win)
{
    QTC_RET_IF_FAIL(qtc_xcb_conn && win);
    static const uint32_t val = XCB_STACK_MODE_ABOVE;
    qtcX11CallVoid(configure_window, win, XCB_CONFIG_WINDOW_STACK_MODE, &val);
    qtcX11CallVoid(map_window, win);
}

QTC_EXPORT bool
qtcX11CompositingActive()
{
    QTC_RET_IF_FAIL(qtc_xcb_conn, false);
    xcb_get_selection_owner_reply_t *reply =
        qtcX11Call(get_selection_owner, qtc_x11_net_wm_cm_s_default);
    QTC_RET_IF_FAIL(reply, false);
    bool res = (reply->owner != 0);
    free(reply);
    return res;
}

QTC_EXPORT bool
qtcX11HasAlpha(xcb_window_t win)
{
    QTC_RET_IF_FAIL(qtc_xcb_conn && win, false);
    if (!qtcX11CompositingActive()) {
        return false;
    }
    xcb_get_geometry_reply_t *reply = qtcX11Call(get_geometry, win);
    QTC_RET_IF_FAIL(reply, false);
    bool res = (reply->depth == 32);
    free(reply);
    return res;
}

QTC_EXPORT bool
qtcX11IsEmbed(xcb_window_t win)
{
    QTC_RET_IF_FAIL(qtc_xcb_conn && win, false);
    xcb_get_property_reply_t *reply =
        qtcX11GetProperty(0, win, qtc_x11_xembed_info,
                          qtc_x11_xembed_info, 0, 1);
    QTC_RET_IF_FAIL(reply, false);
    bool res = xcb_get_property_value_length(reply) > 0;
    free(reply);
    return res;
}

#if 0
QTC_EXPORT void*
qtcX11RgbaVisual(unsigned long *colormap, int *map_entries, int screen)
{
    QTC_RET_IF_FAIL(qtc_disp, NULL);
    if (screen < 0) {
        screen = qtc_default_screen_no;
    }
    // Copied from Qt4
    Visual *argbVisual = NULL;
    int nvi;
    XVisualInfo templ;
    templ.screen = screen;
    templ.depth = 32;
    templ.class = TrueColor;
    XVisualInfo *xvi = XGetVisualInfo(qtc_disp, VisualScreenMask |
                                      VisualDepthMask | VisualClassMask,
                                      &templ, &nvi);
    for (int idx = 0; idx < nvi; ++idx) {
        XRenderPictFormat *format = XRenderFindVisualFormat(qtc_disp,
                                                            xvi[idx].visual);
        if (format->type == PictTypeDirect && format->direct.alphaMask) {
            argbVisual = xvi[idx].visual;
            break;
        }
    }
    XFree(xvi);
    QTC_RET_IF_FAIL(argbVisual, NULL);
    qtcAssign(colormap, XCreateColormap(qtc_disp, RootWindow(qtc_disp, screen),
                                        argbVisual, AllocNone));
    qtcAssign(map_entries, argbVisual->map_entries);
    return argbVisual;
}
#endif

#else

QTC_EXPORT bool
qtcX11Enabled()
{
    return false;
}

QTC_EXPORT xcb_window_t
qtcX11RootWindow(int scrn_no)
{
    QTC_UNUSED(scrn_no);
    return 0;
}

QTC_EXPORT int
qtcX11DefaultScreenNo()
{
    return -1;
}

QTC_EXPORT xcb_screen_t*
qtcX11DefaultScreen()
{
    return NULL;
}

QTC_EXPORT xcb_screen_t*
qtcX11GetScreen(int screen_no)
{
    QTC_UNUSED(screen_no);
    return NULL;
}

QTC_EXPORT void
qtcX11InitXcb(xcb_connection_t *conn, int screen_no)
{
    QTC_UNUSED(conn);
    QTC_UNUSED(screen_no);
}

QTC_EXPORT void
qtcX11InitXlib(void *disp)
{
    QTC_UNUSED(disp);
}

QTC_EXPORT xcb_connection_t*
qtcX11GetConn()
{
    return NULL;
}

QTC_EXPORT void*
qtcX11GetDisp()
{
    return NULL;
}

QTC_EXPORT void
qtcX11MapRaised(xcb_window_t win)
{
    QTC_UNUSED(win);
}

QTC_EXPORT bool
qtcX11CompositingActive()
{
    return false;
}

QTC_EXPORT bool
qtcX11HasAlpha(xcb_window_t win)
{
    QTC_UNUSED(win);
    return false;
}

QTC_EXPORT bool
qtcX11IsEmbed(xcb_window_t win)
{
    QTC_UNUSED(win);
    return false;
}

#if 0
QTC_EXPORT void*
qtcX11RgbaVisual(unsigned long *colormap, int *map_entries, int screen)
{
    QTC_UNUSED(colormap);
    QTC_UNUSED(map_entries);
    QTC_UNUSED(screen);
    return NULL;
}
#endif

#endif
