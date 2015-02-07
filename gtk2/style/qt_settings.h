/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#ifndef __QT_SETTINGS_H__
#define __QT_SETTINGS_H__

#include <gtk/gtk.h>

#include <common/common.h>

namespace QtCurve {

#define READ_INACTIVE_PAL /* Control whether QtCurve should read the inactive palette as well.. */

#define DEBUG_PREFIX "QtCurve: "
#define RC_SETTING "QtC__"

#define COL_EQ(A, B)(abs(A-B)<(3<<8))

#define EQUAL_COLOR(A, B) \
   (COL_EQ(A.red, B.red) && COL_EQ(A.green, B.green) && COL_EQ(A.blue, B.blue))

#define toQtColor(col) \
    ((col&0xFF00)>>8)
/*    ((int)((((double)col)/256.0)+0.5))*/

#define toGtkColor(col) \
    ((col<<8)+col)

typedef struct {
    int smlTbSize,
        tbSize,
        dndSize,
        btnSize,
        mnuSize,
        dlgSize;
} QtIcons;

typedef enum {
    COLOR_BACKGROUND,
    COLOR_BUTTON,
    COLOR_SELECTED,
    COLOR_WINDOW,

    COLOR_MID,
    COLOR_TEXT,
    COLOR_TEXT_SELECTED,
    COLOR_LV,

    COLOR_TOOLTIP,

    COLOR_BUTTON_TEXT,
    COLOR_WINDOW_TEXT,
    COLOR_TOOLTIP_TEXT,

    COLOR_FOCUS,    /* KDE4 */
    COLOR_HOVER,    /* KDE4 */
    COLOR_WINDOW_BORDER,
    COLOR_WINDOW_BORDER_TEXT,

    COLOR_NUMCOLORS,
    COLOR_NUMCOLORS_STD = COLOR_NUMCOLORS-2 /* Remove Window border colors */
} QtColorRoles;

typedef enum {
    GTK_APP_UNKNOWN,
    GTK_APP_MOZILLA,
    GTK_APP_NEW_MOZILLA, /* For firefox3 */
    GTK_APP_OPEN_OFFICE,
    GTK_APP_VMPLAYER,
    GTK_APP_GIMP,
    GTK_APP_JAVA,
    GTK_APP_JAVA_SWT,
    GTK_APP_EVOLUTION,
    GTK_APP_GHB
} EGtkApp;

typedef enum {
    PAL_ACTIVE,
    PAL_DISABLED,
    PAL_INACTIVE
#ifndef READ_INACTIVE_PAL
        = PAL_ACTIVE
#endif
    ,

    PAL_NUMPALS
} QtPallete;

typedef enum {
    FONT_GENERAL,
    FONT_MENU,
    FONT_TOOLBAR,

    FONT_NUM_STD,

    FONT_BOLD = FONT_NUM_STD,

    FONT_NUM_TOTAL
} QtFont;

typedef enum {
    DEBUG_NONE,
    DEBUG_SETTINGS,
    DEBUG_ALL,
} QtcDebug;

typedef struct {
    GdkColor        colors[PAL_NUMPALS][COLOR_NUMCOLORS]; /*,
                    inactiveSelectCol;*/
    char *fonts[FONT_NUM_TOTAL];
    char *icons;
#ifdef QTC_GTK2_STYLE_SUPPORT
    char *styleName;
#endif
    const char *appName;
    GtkToolbarStyle toolbarStyle;
    QtIcons         iconSizes;
    bool buttonIcons;
    bool shadeSortedList;
    EGtkApp app;
    bool qt4;
    bool inactiveChangeSelectionColor;
    bool useAlpha;
    // int startDragDist;
    int startDragTime;
    QtcDebug debug;
} QtData;

typedef struct {
    GdkColor background[TOTAL_SHADES+1],
             button[2][TOTAL_SHADES+1],
             *slider,
             *defbtn,
             *mouseover,
             *combobtn,
             *selectedcr,
             *sortedlv,
             *sidebar,
             *progress,
             *wborder[2],
             mdi_text[2],
             menubar[TOTAL_SHADES+1],
             highlight[TOTAL_SHADES+1],
             focus[TOTAL_SHADES+1],
             menu[TOTAL_SHADES+1],
             *check_radio;
} QtCPalette;

extern QtCPalette qtcPalette;
extern Options opts;
extern QtData qtSettings;

bool qtSettingsInit();
void qtSettingsSetColors(GtkStyle *style, GtkRcStyle *rc_style);

}

#endif
