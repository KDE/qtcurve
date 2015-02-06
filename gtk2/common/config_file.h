/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef QTC_CONFIG_FILE_H
#define QTC_CONFIG_FILE_H

#include "common.h"

QTC_BEGIN_DECLS

#define MAX_CONFIG_FILENAME_LEN   1024
#define MAX_CONFIG_INPUT_LINE_LEN 256
#define QTC_MENU_FILE_PREFIX   "menubar-"
#define QTC_STATUS_FILE_PREFIX "statusbar-"

#define qtcMenuBarHidden(A) qtcBarHidden((A), QTC_MENU_FILE_PREFIX)
#define qtcSetMenuBarHidden(A, H)                       \
    qtcSetBarHidden((A), (H), QTC_MENU_FILE_PREFIX)
#define qtcStatusBarHidden(A)                   \
    qtcBarHidden((A), QTC_STATUS_FILE_PREFIX)
#define qtcSetStatusBarHidden(A, H)                     \
    qtcSetBarHidden((A), (H), QTC_STATUS_FILE_PREFIX)

bool qtcBarHidden(const char *app, const char *prefix);
void qtcSetBarHidden(const char *app, bool hidden, const char *prefix);
void qtcLoadBgndImage(QtCImage *img);

void qtcSetRgb(GdkColor *col, const char *str);
void qtcDefaultSettings(Options *opts);
void qtcCheckConfig(Options *opts);
bool qtcReadConfig(const char *file, Options *opts, Options *defOpts);
WindowBorders qtcGetWindowBorderSize(bool force);

QTC_END_DECLS

#endif
