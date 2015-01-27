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

#include "x11utils.h"

#ifndef _QTC_UTILS_X11UTILS_P_H_
#define _QTC_UTILS_X11UTILS_P_H_

QTC_BEGIN_DECLS

extern QtcXDisplay *qtc_disp;
extern xcb_connection_t *qtc_xcb_conn;
extern int qtc_default_screen_no;
extern xcb_window_t qtc_root_window;
extern xcb_screen_t *qtc_default_screen;
extern xcb_atom_t qtc_x11_kde_net_wm_blur_behind_region;
extern xcb_atom_t qtc_x11_kde_net_wm_shadow;
extern xcb_atom_t qtc_x11_net_wm_moveresize;
extern xcb_atom_t qtc_x11_net_wm_cm_s_default;

QTC_END_DECLS

#ifndef __cplusplus
#define qtcX11Call(name, args...)                                       \
    ({                                                                  \
        xcb_connection_t *conn = qtc_xcb_conn;                          \
        xcb_##name##_reply_t *res = NULL;                               \
        if (qtcLikely(conn)) {                                          \
            res = xcb_##name##_reply(conn, xcb_##name(conn, args), 0);  \
        }                                                               \
        res;                                                            \
    })
#define qtcX11CallVoid(name, args...)                   \
    ({                                                  \
        xcb_connection_t *conn = qtc_xcb_conn;          \
        xcb_void_cookie_t res = {0};                    \
        if (qtcLikely(conn)) {                          \
            res = xcb_##name(conn, args);               \
        }                                               \
        res;                                            \
    })
#define qtcX11CallVoidChecked(name, args...)            \
    ({                                                  \
        xcb_connection_t *conn = qtc_xcb_conn;          \
        xcb_void_cookie_t res = {0};                    \
        if (qtcLikely(conn)) {                          \
            xcb_##name##_checked(conn, args);           \
        }                                               \
        res;                                            \
    })
#else

template <typename Ret, typename Cookie, typename... Args, typename... Args2>
static inline Ret*
_qtcX11Call(Cookie (*func)(xcb_connection_t*, Args...),
            Ret *(reply_func)(xcb_connection_t*, Cookie, xcb_generic_error_t**),
            Args2... args...)
{
    xcb_connection_t *conn = qtc_xcb_conn;
    QTC_RET_IF_FAIL(conn, NULL);
    Cookie cookie = func(conn, args...);
    return reply_func(conn, cookie, 0);
}
#define qtcX11Call(name, args...)                       \
    (_qtcX11Call(xcb_##name, xcb_##name##_reply, args))

template <typename... Args, typename... Args2>
static inline xcb_void_cookie_t
_qtcX11CallVoid(xcb_void_cookie_t (*func)(xcb_connection_t*, Args...),
                Args2... args...)
{
    xcb_connection_t *conn = qtc_xcb_conn;
    QTC_RET_IF_FAIL(conn, xcb_void_cookie_t());
    return func(conn, args...);
}
#define qtcX11CallVoid(name, args...)           \
    (_qtcX11CallVoid(xcb_##name, args))

#define qtcX11CallVoidChecked(name, args...)            \
    (_qtcX11CallVoid(xcb_##name##_checked, args))

#endif

QTC_ALWAYS_INLINE static inline xcb_atom_t
qtcX11GetAtom(const char *name, bool create)
{
    xcb_intern_atom_reply_t *r = qtcX11Call(intern_atom, !create,
                                            strlen(name), name);
    xcb_atom_t atom = r ? r->atom : 0;
    qtcFree(r);
    return atom;
}

#endif
