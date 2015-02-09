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

#include "config.h"

#include "x11shadow_p.h"
#include "x11wmmove.h"
#include "x11blur.h"
#include "x11qtc.h"
#include "x11wrap.h"

#ifdef QTC_ENABLE_X11

#include "log.h"
#include "number.h"
#include "shadow_p.h"
#include "x11utils_p.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>

/**
 * shadow atom and property specification available at
 * http://community.kde.org/KWin/Shadow
 **/

static uint32_t shadow_xpixmaps[8];
static uint32_t shadow_data_xcb[8 + 4];
/**
 * Use XCB to set window property receives BadWindow errors for menus in
 * Qt4 kpartsplugin here, probably because of the order of some pending
 * event/requests in Xlib. Calling #XFlush before #xcb_change_property
 * doesn't solve the problem for unknown reason but using #XChangeProperty
 * works.
 * NOTE: #XChangeProperty want `unsigned long` for format 32. So we need
 * two separate data buffers.
 **/
static unsigned long shadow_data_xlib[8 + 4];

static xcb_pixmap_t
qtcX11ShadowCreatePixmap(const QtcImage *data)
{
    xcb_pixmap_t pixmap = qtcX11GenerateId();

    // create X11 pixmap
    qtcX11CallVoid(create_pixmap, 32, pixmap, qtc_root_window,
                   data->width, data->height);
    xcb_gcontext_t cid = qtcX11GenerateId();
    qtcX11CallVoid(create_gc, cid, pixmap, 0, (const uint32_t*)0);
    qtcX11CallVoid(put_image, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap, cid,
                   data->width, data->height, 0, 0, 0, 32, data->len,
                   (unsigned char*)data->data);
    qtcX11CallVoid(free_gc, cid);
    qtcX11Flush();
    return pixmap;
}

void
qtcX11ShadowInit()
{
    int shadow_size = 30;
    int shadow_radius = 4;
    QtcColor c1 = {0.4, 0.4, 0.4};
    QtcColor c2 = {0.2, 0.2, 0.2};
    QtcImage *shadow_images[8];
    qtcShadowCreate(shadow_size, &c1, &c2, shadow_radius, false,
                    QTC_PIXEL_XCB, shadow_images);
    for (int i = 0;i < 8;i++) {
        shadow_xpixmaps[i] = qtcX11ShadowCreatePixmap(shadow_images[i]);
        free(shadow_images[i]);
    }

    memcpy(shadow_data_xcb, shadow_xpixmaps, sizeof(shadow_xpixmaps));
    for (int i = 0;i < 8;i++) {
        shadow_data_xlib[i] = shadow_xpixmaps[i];
    }
    for (int i = 8;i < 12;i++) {
        shadow_data_xlib[i] = shadow_data_xcb[i] = shadow_size - 1;
    }
}

// Necessary?
void
qtcX11ShadowDestroy()
{
    QTC_RET_IF_FAIL(qtc_xcb_conn);
    for (unsigned int i = 0;
         i < sizeof(shadow_xpixmaps) / sizeof(shadow_xpixmaps[0]);i++) {
        qtcX11CallVoid(free_pixmap, shadow_xpixmaps[i]);
    }
    qtcX11Flush();
}

QTC_EXPORT void
qtcX11ShadowInstall(xcb_window_t win, const int margins[4])
{
    QTC_RET_IF_FAIL(win);
    if (qtcUnlikely(!margins)) {
        qtcX11ShadowInstall(win);
        return;
    }
    // In principle, I should check for _KDE_NET_WM_SHADOW in _NET_SUPPORTED.
    // However, it's complicated and we will gain nothing.
    xcb_atom_t atom = qtc_x11_kde_net_wm_shadow;
    if (qtc_disp) {
        unsigned long shadow_data[8 + 4];
        memcpy(shadow_data, shadow_data_xlib, 12 * sizeof(unsigned long));
        for (int i = 0;i < 4;i++) {
            shadow_data[i + 8] -= margins[i];
        }
        XChangeProperty((Display*)qtc_disp, win, atom, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char*)shadow_data, 12);
    } else {
        uint32_t shadow_data[8 + 4];
        memcpy(shadow_data, shadow_data_xcb, 12 * sizeof(uint32_t));
        for (int i = 0;i < 4;i++) {
            shadow_data[i + 8] -= margins[i];
        }
        qtcX11ChangeProperty(XCB_PROP_MODE_REPLACE, win, atom,
                             XCB_ATOM_CARDINAL, 32, 12, shadow_data);
        qtcX11Flush();
    }
}

QTC_EXPORT void
qtcX11ShadowInstall(xcb_window_t win)
{
    QTC_RET_IF_FAIL(win);
    // In principle, I should check for _KDE_NET_WM_SHADOW in _NET_SUPPORTED.
    // However, it's complicated and we will gain nothing.
    xcb_atom_t atom = qtc_x11_kde_net_wm_shadow;
    if (qtc_disp) {
        XChangeProperty((Display*)qtc_disp, win, atom, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char*)shadow_data_xlib, 12);
    } else {
        qtcX11ChangeProperty(XCB_PROP_MODE_REPLACE, win, atom,
                             XCB_ATOM_CARDINAL, 32, 12, shadow_data_xcb);
        qtcX11Flush();
    }
}

QTC_EXPORT void
qtcX11ShadowUninstall(xcb_window_t win)
{
    QTC_RET_IF_FAIL(win);
    qtcX11CallVoid(delete_property, win, qtc_x11_kde_net_wm_shadow);
    qtcX11Flush();
}

// WM Move
QTC_EXPORT void
qtcX11MoveTrigger(xcb_window_t wid, uint32_t x, uint32_t y)
{
    QTC_RET_IF_FAIL(wid);
    qtcX11FlushXlib();
    qtcX11CallVoid(ungrab_pointer, XCB_TIME_CURRENT_TIME);
    union {
        char _buff[32];
        xcb_client_message_event_t ev;
    } buff;
    memset(&buff, 0, sizeof(buff));
    // ...Taken from bespin...
    // stolen... errr "adapted!" from QSizeGrip
    // Well now it is "ported"
    xcb_client_message_event_t *xev = &buff.ev;
    xev->response_type = XCB_CLIENT_MESSAGE;
    xev->format = 32;
    xev->window = wid;
    xev->type = qtc_x11_net_wm_moveresize;
    xev->data.data32[0] = x;
    xev->data.data32[1] = y;
    xev->data.data32[2] = 8; // NET::Move
    xev->data.data32[3] = XCB_KEY_BUT_MASK_BUTTON_1;
    qtcX11SendEvent(false, qtc_root_window,
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, xev);
    qtcX11Flush();
}

// Blur
QTC_EXPORT void
qtcX11BlurTrigger(xcb_window_t wid, bool enable, unsigned prop_num,
                  const uint32_t *props)
{
    QTC_RET_IF_FAIL(wid);
    xcb_atom_t atom = qtc_x11_kde_net_wm_blur_behind_region;
    if (enable) {
        if (qtc_disp) {
            QTC_DEF_LOCAL_BUFF(unsigned long, xlib_props, 256, prop_num);
            for (unsigned i = 0;i < prop_num;i++) {
                xlib_props.p[i] = props[i];
            }
            XChangeProperty((Display*)qtc_disp, wid, atom, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char*)xlib_props.p,
                            prop_num);
            QTC_FREE_LOCAL_BUFF(xlib_props);
        } else {
            qtcX11ChangeProperty(XCB_PROP_MODE_REPLACE, wid, atom,
                                 XCB_ATOM_CARDINAL, 32, prop_num, props);
        }
    } else {
        qtcX11CallVoid(delete_property, wid, atom);
    }
    qtcX11Flush();
}

QTC_EXPORT int32_t
qtcX11GetShortProp(xcb_window_t win, xcb_atom_t atom)
{
    QTC_RET_IF_FAIL(qtc_xcb_conn && win, -1);
    int32_t res = -1;
    xcb_get_property_reply_t *reply =
        qtcX11GetProperty(0, win, atom, XCB_ATOM_CARDINAL, 0, 1);
    QTC_RET_IF_FAIL(reply, -1);
    if (xcb_get_property_value_length(reply) > 0) {
        uint32_t val = *(int32_t*)xcb_get_property_value(reply);
        if (val < 512) {
            res = val;
        }
    }
    free(reply);
    return res;
}

static inline void
qtcX11SetShortProp(xcb_window_t win, xcb_atom_t atom, unsigned short prop)
{
    qtcX11ChangeProperty(XCB_PROP_MODE_REPLACE, win, atom,
                         XCB_ATOM_CARDINAL, 16, 1, &prop);
    qtcX11Flush();
}

QTC_EXPORT void
qtcX11SetMenubarSize(xcb_window_t win, unsigned short s)
{
    qtcX11SetShortProp(win, qtc_x11_qtc_menubar_size, s);
}

QTC_EXPORT void
qtcX11SetStatusBar(xcb_window_t win)
{
    qtcX11SetShortProp(win, qtc_x11_qtc_statusbar, 1);
}

QTC_EXPORT void
qtcX11SetOpacity(xcb_window_t win, unsigned short o)
{
    qtcX11SetShortProp(win, qtc_x11_qtc_opacity, o);
}

QTC_EXPORT void
qtcX11SetBgnd(xcb_window_t win, uint32_t prop)
{
    qtcX11ChangeProperty(XCB_PROP_MODE_REPLACE, win, qtc_x11_qtc_bgnd,
                         XCB_ATOM_CARDINAL, 32, 1, &prop);
    qtcX11Flush();
}

#else

QTC_EXPORT void
qtcX11ShadowInstall(xcb_window_t, const int[4])
{
    QTC_UNUSED(win);
    QTC_UNUSED(margins);
}

QTC_EXPORT void
qtcX11ShadowInstall(xcb_window_t win)
{
    QTC_UNUSED(win);
}

QTC_EXPORT void
qtcX11ShadowUninstall(xcb_window_t win)
{
    QTC_UNUSED(win);
}

// WM Move
QTC_EXPORT void
qtcX11MoveTrigger(xcb_window_t wid, uint32_t x, uint32_t y)
{
    QTC_UNUSED(wid);
    QTC_UNUSED(x);
    QTC_UNUSED(y);
}

// Blur
QTC_EXPORT void
qtcX11BlurTrigger(xcb_window_t wid, bool enable, unsigned prop_num,
                  const uint32_t *props)
{
    QTC_UNUSED(wid);
    QTC_UNUSED(enable);
    QTC_UNUSED(prop_num);
    QTC_UNUSED(props);
}

QTC_EXPORT int32_t
qtcX11GetShortProp(xcb_window_t win, xcb_atom_t atom)
{
    QTC_UNUSED(win);
    QTC_UNUSED(atom);
    return -1;
}

QTC_EXPORT void
qtcX11SetMenubarSize(xcb_window_t win, unsigned short s)
{
    QTC_UNUSED(win);
    QTC_UNUSED(s);
}

QTC_EXPORT void
qtcX11SetStatusBar(xcb_window_t win)
{
    QTC_UNUSED(win);
}

QTC_EXPORT void
qtcX11SetOpacity(xcb_window_t win, unsigned short o)
{
    QTC_UNUSED(win);
    QTC_UNUSED(o);
}

QTC_EXPORT void
qtcX11SetBgnd(xcb_window_t win, uint32_t prop)
{
    QTC_UNUSED(win);
    QTC_UNUSED(prop);
}

#endif
