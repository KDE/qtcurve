/*****************************************************************************
 *   Copyright 2014 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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
#include "x11wrap.h"

#ifdef QTC_ENABLE_X11

#include "x11utils_p.h"
#include <X11/Xlib.h>

QTC_EXPORT void
qtcX11FlushXlib()
{
    QTC_RET_IF_FAIL(qtc_disp);
    XFlush((Display*)qtc_disp);
}

QTC_EXPORT void
qtcX11Flush()
{
    QTC_RET_IF_FAIL(qtc_xcb_conn);
    xcb_flush(qtc_xcb_conn);
}

QTC_EXPORT uint32_t
qtcX11GenerateId()
{
    QTC_RET_IF_FAIL(qtc_xcb_conn, 0);
    return xcb_generate_id(qtc_xcb_conn);
}

QTC_EXPORT void
qtcX11ChangeProperty(uint8_t mode, xcb_window_t win, xcb_atom_t prop,
                     xcb_atom_t type, uint8_t format, uint32_t len,
                     const void *data)
{
    qtcX11CallVoid(change_property, mode, win, prop, type, format, len, data);
}

QTC_EXPORT xcb_query_tree_reply_t*
qtcX11QueryTree(xcb_window_t win)
{
    return qtcX11Call(query_tree, win);
}

QTC_EXPORT void
qtcX11ReparentWindow(xcb_window_t win, xcb_window_t parent,
                     int16_t x, int16_t y)
{
    qtcX11CallVoid(reparent_window, win, parent, x, y);
}

QTC_EXPORT void
qtcX11SendEvent(uint8_t propagate, xcb_window_t destination,
                uint32_t event_mask, const void *event)
{
    qtcX11CallVoid(send_event, propagate, destination, event_mask,
                   (const char*)event);
}

QTC_EXPORT xcb_get_property_reply_t*
qtcX11GetProperty(uint8_t del, xcb_window_t win, xcb_atom_t prop,
                  xcb_atom_t type, uint32_t offset, uint32_t len)
{
    return qtcX11Call(get_property, del, win, prop, type, offset, len);
}

QTC_EXPORT void*
qtcX11GetPropertyValue(const xcb_get_property_reply_t *reply)
{
    return xcb_get_property_value(reply);
}

QTC_EXPORT int
qtcX11GetPropertyValueLength(const xcb_get_property_reply_t *reply)
{
    return xcb_get_property_value_length(reply);
}

#else

QTC_EXPORT void
qtcX11FlushXlib()
{
}

QTC_EXPORT void
qtcX11Flush()
{
}

QTC_EXPORT uint32_t
qtcX11GenerateId()
{
    return 0;
}

QTC_EXPORT void
qtcX11ChangeProperty(uint8_t mode, xcb_window_t win, xcb_atom_t prop,
                     xcb_atom_t type, uint8_t format, uint32_t len,
                     const void *data)
{
    QTC_UNUSED(mode);
    QTC_UNUSED(win);
    QTC_UNUSED(prop);
    QTC_UNUSED(type);
    QTC_UNUSED(format);
    QTC_UNUSED(len);
    QTC_UNUSED(data);
}

QTC_EXPORT xcb_query_tree_reply_t*
qtcX11QueryTree(xcb_window_t win)
{
    QTC_UNUSED(win);
    return NULL;
}

QTC_EXPORT void
qtcX11ReparentWindow(xcb_window_t win, xcb_window_t parent,
                     int16_t x, int16_t y)
{
    QTC_UNUSED(win);
    QTC_UNUSED(parent);
    QTC_UNUSED(x);
    QTC_UNUSED(y);
}

QTC_EXPORT void
qtcX11SendEvent(uint8_t propagate, xcb_window_t destination,
                uint32_t event_mask, const void *event)
{
    QTC_UNUSED(propagate);
    QTC_UNUSED(destination);
    QTC_UNUSED(event_mask);
    QTC_UNUSED(event);
}

QTC_EXPORT xcb_get_property_reply_t*
qtcX11GetProperty(uint8_t del, xcb_window_t win, xcb_atom_t prop,
                  xcb_atom_t type, uint32_t offset, uint32_t len)
{
    QTC_UNUSED(del);
    QTC_UNUSED(win);
    QTC_UNUSED(prop);
    QTC_UNUSED(type);
    QTC_UNUSED(offset);
    QTC_UNUSED(len);
    return NULL;
}

QTC_EXPORT void*
qtcX11GetPropertyValue(const xcb_get_property_reply_t *reply)
{
    QTC_UNUSED(reply);
    return NULL;
}

QTC_EXPORT int
qtcX11GetPropertyValueLength(const xcb_get_property_reply_t *reply)
{
    QTC_UNUSED(reply);
    return 0;
}

#endif
