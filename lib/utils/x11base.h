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

#ifndef _QTC_UTILS_X11BASE_H_
#define _QTC_UTILS_X11BASE_H_

#include <xcb/xcb.h>
#include "utils.h"

extern xcb_atom_t qtc_x11_qtc_menubar_size;
extern xcb_atom_t qtc_x11_qtc_statusbar;
extern xcb_atom_t qtc_x11_qtc_titlebar_size;
extern xcb_atom_t qtc_x11_qtc_active_window;
extern xcb_atom_t qtc_x11_qtc_toggle_menubar;
extern xcb_atom_t qtc_x11_qtc_toggle_statusbar;
extern xcb_atom_t qtc_x11_qtc_opacity;
extern xcb_atom_t qtc_x11_qtc_bgnd;

bool qtcX11Enabled();
void qtcX11InitXcb(xcb_connection_t *conn, int screen_no);
void qtcX11InitXlib(void *disp);
xcb_connection_t *qtcX11GetConn();
void *qtcX11GetDisp();
int qtcX11DefaultScreenNo();
xcb_screen_t *qtcX11DefaultScreen();
xcb_screen_t *qtcX11GetScreen(int scrn_no=-1);
xcb_window_t qtcX11RootWindow(int scrn_no=-1);

#endif
