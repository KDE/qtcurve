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

#include <stdint.h>

typedef uint32_t xcb_atom_t;
typedef struct xcb_connection_t xcb_connection_t;
typedef struct xcb_screen_t xcb_screen_t;
typedef uint32_t xcb_window_t;
typedef struct xcb_query_tree_reply_t xcb_query_tree_reply_t;
typedef struct xcb_get_property_reply_t xcb_get_property_reply_t;
#define XCB_ATOM_CARDINAL 6
#define XCB_PROP_MODE_REPLACE 0
