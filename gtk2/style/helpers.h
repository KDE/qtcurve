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

#ifndef __QTC_HELPERS_H__
#define __QTC_HELPERS_H__

#include <common/common.h>
#include "config.h"
#include "qt_settings.h"
#include <qtcurve-cairo/utils.h>

QTC_BEGIN_DECLS

#define DETAIL(xx) ((detail) && (!strcmp(xx, detail)))
#define DETAILHAS(xx) ((detail) && (strstr(detail, xx)))

#ifndef GTK_IS_COMBO_BOX_ENTRY
#define GTK_IS_COMBO_BOX_ENTRY(x) 0
#endif
#ifndef GTK_IS_COMBO_BOX
#define GTK_IS_COMBO_BOX(x) 0
#endif

#if GTK_CHECK_VERSION(2, 90, 0)
#  define QTC_IS_COMBO(X) GTK_IS_COMBO_BOX_TEXT(X)
#  define QTC_COMBO_ENTRY(X) GTK_IS_COMBO_BOX_TEXT(X)
#elif GTK_CHECK_VERSION(2, 24, 0)
#  define QTC_IS_COMBO(X) (GTK_IS_COMBO(X) || GTK_IS_COMBO_BOX_TEXT(X))
#  define QTC_COMBO_ENTRY(X) (GTK_IS_COMBO_BOX_ENTRY(X) ||      \
                              GTK_IS_COMBO_BOX_TEXT(X))
#else
#  define QTC_IS_COMBO(X) GTK_IS_COMBO(X)
#  define QTC_COMBO_ENTRY(X) GTK_IS_COMBO_BOX_ENTRY(X)
#endif

#define QT_CUSTOM_COLOR_BUTTON(style)           \
    (style &&                                                           \
     !(COL_EQ(qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW].red,(style->bg[GTK_STATE_SELECTED==state ? state : GTK_STATE_NORMAL].red)) && \
      COL_EQ(qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW].green,(style->bg[GTK_STATE_SELECTED==state ? state : GTK_STATE_NORMAL].green)) && \
      COL_EQ(qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW].blue,(style->bg[GTK_STATE_SELECTED==state ? state : GTK_STATE_NORMAL].blue))))

#define SET_BTN_COLS(SCROLLBAR, SCALE, LISTVIEW, STATE) \
{ \
    if(SCROLLBAR || SCALE) \
        btnColors=GTK_STATE_INSENSITIVE==STATE \
                    ? qtcPalette.background \
                    : SHADE_NONE!=opts.shadeSliders && qtcPalette.slider && \
                      (!opts.colorSliderMouseOver || GTK_STATE_PRELIGHT==STATE) \
                        ? qtcPalette.slider \
                        : qtcPalette.button[PAL_ACTIVE]; \
    else if(LISTVIEW) \
    { \
        if(GTK_STATE_INSENSITIVE!=state && qtcPalette.sortedlv && isSortColumn(widget)) \
            btnColors=qtcPalette.sortedlv;  \
        else if(opts.lvButton) \
            btnColors=qtcPalette.button[GTK_STATE_INSENSITIVE==STATE ? PAL_DISABLED : PAL_ACTIVE]; \
        else \
            btnColors=qtcPalette.background; \
    } \
    else \
        btnColors=qtcPalette.button[GTK_STATE_INSENSITIVE==STATE ? PAL_DISABLED : PAL_ACTIVE]; \
}

#define ARROW_STATE(state) (GTK_STATE_INSENSITIVE==state ? state : GTK_STATE_NORMAL)
/* (GTK_STATE_ACTIVE==state ? GTK_STATE_NORMAL : state) */

void debugDisplayWidget(GtkWidget *widget, int level);
bool haveAlternateListViewCol();
bool isFixedWidget(GtkWidget *widget);
QTC_ALWAYS_INLINE static inline bool
isMozilla()
{
    return (qtcOneOf(qtSettings.app, GTK_APP_MOZILLA, GTK_APP_NEW_MOZILLA) &&
            !getenv("QTCURVE_MOZ_TEST"));
}
QTC_ALWAYS_INLINE static inline bool
isMozillaTab(GtkWidget *widget)
{
    return isFixedWidget(widget) && GTK_IS_NOTEBOOK(widget);
}
QTC_ALWAYS_INLINE static inline bool
isFakeGtk()
{
    return (isMozilla() || qtSettings.app == GTK_APP_OPEN_OFFICE ||
            qtSettings.app == GTK_APP_JAVA);
}
GdkColor *menuColors(bool active);
EBorder shadowToBorder(GtkShadowType shadow);
bool useButtonColor(const char *detail);
void qtcShadeColors(const GdkColor *base, GdkColor *vals);
bool isSortColumn(GtkWidget *button);
GdkColor *getCellCol(GdkColor *std, const char *detail);
bool reverseLayout(GtkWidget *widget);
bool isOnToolbar(GtkWidget *widget, bool *horiz, int level);
bool isOnHandlebox(GtkWidget *widget, bool *horiz, int level);
bool isButtonOnToolbar(GtkWidget *widget, bool *horiz);
bool isButtonOnHandlebox(GtkWidget *widget, bool *horiz);
bool isOnStatusBar(GtkWidget *widget, int level);
bool isList(GtkWidget *widget);
bool isListViewHeader(GtkWidget *widget);
bool isEvolutionListViewHeader(GtkWidget *widget, const char *detail);
bool isOnListViewHeader(GtkWidget *w, int level);
bool isPathButton(GtkWidget *widget);
GtkWidget *getComboEntry(GtkWidget *widget);
GtkWidget *getComboButton(GtkWidget *widget);
bool isSideBarBtn(GtkWidget *widget);
bool isComboBoxButton(GtkWidget *widget);
bool isComboBox(GtkWidget *widget);
bool isComboBoxEntry(GtkWidget *widget);
bool isComboBoxEntryButton(GtkWidget *widget);
bool isGimpCombo(GtkWidget *widget);
bool isOnComboEntry(GtkWidget *w, int level);
bool isOnComboBox(GtkWidget *w, int level);
bool isOnCombo(GtkWidget *w, int level);
#if !GTK_CHECK_VERSION(2, 90, 0)
bool isOnOptionMenu(GtkWidget *w, int level);
bool isActiveOptionMenu(GtkWidget *widget);
#endif
bool isOnMenuItem(GtkWidget *w, int level);
bool isSpinButton(GtkWidget *widget);
bool isStatusBarFrame(GtkWidget *widget);
GtkMenuBar *isMenubar(GtkWidget *w, int level);
bool isMenuitem(GtkWidget *w, int level);
#define IS_MENU_ITEM(WIDGET) isMenuitem(WIDGET, 0)
bool isMenuWindow(GtkWidget *w);
#define IS_GROUP_BOX(W) ((W) && GTK_IS_FRAME((W)) && (NULL!=gtk_frame_get_label(GTK_FRAME((W))) || \
                                                      NULL!=gtk_frame_get_label_widget(GTK_FRAME((W)))))

bool isInGroupBox(GtkWidget *w, int level);
bool isOnButton(GtkWidget *w, int level, bool *def);
void optionMenuGetProps(GtkWidget *widget, GtkRequisition *indicator_size, GtkBorder *indicator_spacing);

typedef enum {
    STEPPER_A,
    STEPPER_B,
    STEPPER_C,
    STEPPER_D,
    STEPPER_NONE
} EStepper;

#if GTK_CHECK_VERSION(2, 90, 0)
#define sanitizeSize(A, B, C) sanitizeSizeReal(widget, B, C)
void gdk_drawable_get_size(GdkWindow *window, int *width, int *height);
void sanitizeSizeReal(GtkWidget *widget, int *width, int *height);
#else
void sanitizeSize(GdkWindow *window, int *width, int *height);
#endif
EStepper getStepper(GtkWidget *widget, int x, int y, int width, int height);

int getFill(GtkStateType state, bool set, bool darker);
#define getFill(state, set, darker...)                  \
    getFill(state, set, QTC_DEFAULT(darker, false))
bool isSbarDetail(const char *detail);
bool isHorizontalProgressbar(GtkWidget *widget);
bool isComboBoxPopupWindow(GtkWidget *widget, int level);
bool isComboBoxList(GtkWidget *widget);
bool isComboPopupWindow(GtkWidget *widget, int level);
bool isComboList(GtkWidget *widget);
bool isComboMenu(GtkWidget *widget);
bool isComboFrame(GtkWidget *widget);
bool isGimpDockable(GtkWidget *widget);
#define isMozillaWidget(widget) (isMozilla() && isFixedWidget(widget))
GdkColor *getParentBgCol(GtkWidget *widget);
int getOpacity(GtkWidget *widget);
void setLowerEtchCol(cairo_t *cr, GtkWidget *widget);
GdkColor shadeColor(const GdkColor *orig, double mod);
gboolean windowEvent(GtkWidget *widget, GdkEvent *event, void *user_data);
void adjustToolbarButtons(GtkWidget *widget, int *x, int *y, int *width, int *height, int *round, bool horiz);
void getEntryParentBgCol(GtkWidget *widget, GdkColor *color);
bool compositingActive(GtkWidget *widget);
bool isRgbaWidget(GtkWidget *widget);
void enableBlurBehind(GtkWidget *w, bool enable);
void getTopLevelSize(GdkWindow *window, int *w, int *h);
void getTopLevelOrigin(GdkWindow *window, int *x, int *y);
bool mapToTopLevel(GdkWindow *window, GtkWidget *widget, int *x, int *y, int *w, int *h); //, bool frame)
int getRound(const char *detail, GtkWidget *widget, bool rev);

bool treeViewCellHasChildren(GtkTreeView *treeView, GtkTreePath *path);
bool treeViewCellIsLast(GtkTreeView *treeView, GtkTreePath *path);
GtkTreePath *treeViewPathParent(GtkTreeView *treeView, GtkTreePath *path);
void generateColors();
GdkColor *getCheckRadioCol(GtkStyle *style, GtkStateType state, bool mnu);

QTC_END_DECLS

#endif
