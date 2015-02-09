/*****************************************************************************
 *   Copyright 2014 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef _QTC_UTILS_X11WRAP_H_
#define _QTC_UTILS_X11WRAP_H_

#include "x11base.h"

void qtcX11Flush();
void qtcX11FlushXlib();
uint32_t qtcX11GenerateId();
void qtcX11ChangeProperty(uint8_t mode, xcb_window_t win, xcb_atom_t prop,
                          xcb_atom_t type, uint8_t format, uint32_t len,
                          const void *data);
xcb_query_tree_reply_t *qtcX11QueryTree(xcb_window_t win);
void qtcX11ReparentWindow(xcb_window_t win, xcb_window_t parent,
                          int16_t x, int16_t y);
void qtcX11SendEvent(uint8_t propagate, xcb_window_t destination,
                     uint32_t event_mask, const void *event);
xcb_get_property_reply_t *qtcX11GetProperty(uint8_t del, xcb_window_t win,
                                            xcb_atom_t prop, xcb_atom_t type,
                                            uint32_t offset, uint32_t len);
void *qtcX11GetPropertyValue(const xcb_get_property_reply_t *reply);
int qtcX11GetPropertyValueLength(const xcb_get_property_reply_t *reply);

#endif
