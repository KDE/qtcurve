/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#include <qtcurve-utils/color.h>
#include <qtcurve-utils/strs.h>
#include <qtcurve-utils/gtkprops.h>
#include <qtcurve-utils/x11base.h>
#include <qtcurve-cairo/draw.h>

#include <gmodule.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <cairo.h>
#include "compatability.h"
#include <common/config_file.h>

#define MO_ARROW(MENU, COL)                             \
    (!MENU && MO_NONE != opts.coloredMouseOver &&       \
     GTK_STATE_PRELIGHT == state ?                      \
     &qtcPalette.mouseover[ARROW_MO_SHADE] : (COL))

#include "qt_settings.h"
#include "animation.h"
#include "menu.h"
#include "tab.h"
#include "widgetmap.h"
#include "window.h"
#include "entry.h"
#include "treeview.h"
#include "combobox.h"
#include "scrolledwindow.h"
#include "scrollbar.h"
#include "wmmove.h"
#include "helpers.h"
#include "drawing.h"
#include "pixcache.h"
#include "shadowhelper.h"
#include "config.h"

namespace QtCurve {

static GType style_type = 0;
static GType rc_style_type = 0;

struct StyleClass {
    GtkStyleClass parent_class;
};

struct Style {
    GtkStyle parent_instance;
    GdkColor *button_text[2];
    GdkColor *menutext[2];
};

struct RcStyleClass {
    GtkRcStyleClass parent_class;
};

struct RcStyle {
    GtkRcStyle parent_instance;
};

template<typename T>
static inline bool
isRcStyle(T *object)
{
    return G_TYPE_CHECK_INSTANCE_TYPE(object, rc_style_type);
}

static GtkStyleClass *parent_class = nullptr;

#ifdef INCREASE_SB_SLIDER
typedef struct {
    GtkStyle *style;
#if GTK_CHECK_VERSION(2, 90, 0)
    cairo_t *cr;
#else
    GdkWindow *window;
#endif
    GtkStateType state;
    GtkShadowType shadow;
    GtkWidget *widget;
    const char *detail;
    int x;
    int y;
    int width;
    int height;
    GtkOrientation orientation;
} QtCSlider;

static QtCSlider lastSlider;
#endif

template<typename Widget>
static inline bool
widgetIsType(Widget *widget, const char *name)
{
    return widget && !strcmp(g_type_name(G_TYPE_FROM_INSTANCE(widget)), name);
}

static void gtkDrawBox(GtkStyle *style, GdkWindow *window, GtkStateType state,
                       GtkShadowType shadow, GdkRectangle *area,
                       GtkWidget *widget, const char *detail, int x, int y,
                       int width, int height);

static void gtkDrawSlider(GtkStyle *style, GdkWindow *window,
                          GtkStateType state, GtkShadowType shadow,
                          GdkRectangle *area, GtkWidget *widget,
                          const char *detail, int x, int y, int width,
                          int height, GtkOrientation orientation);

static void
gtkLogHandler(const char*, GLogLevelFlags, const char*, void*)
{
}

static void
gtkDrawFlatBox(GtkStyle *style, GdkWindow *window, GtkStateType state,
               GtkShadowType shadow, GdkRectangle *area, GtkWidget *widget,
               const char *_detail, int x, int y, int width, int height)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    cairo_t *cr = Cairo::gdkCreateClip(window, area);

    bool isMenuOrToolTipWindow =
        (widget && GTK_IS_WINDOW(widget) &&
         ((gtk_widget_get_name(widget) &&
           strcmp(gtk_widget_get_name(widget), "gtk-tooltip") == 0) ||
          isMenuWindow(widget)));

    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %d %s  ", __FUNCTION__,
               state, shadow, x, y, width, height, _detail);
        debugDisplayWidget(widget, 10);
    }

    sanitizeSize(window, &width, &height);

    if (!opts.gtkButtonOrder && opts.reorderGtkButtons &&
        GTK_IS_WINDOW(widget) && strcmp(detail, "base") == 0) {
        GtkWidget *topLevel = gtk_widget_get_toplevel(widget);
        GtkWidgetProps topProps(topLevel);

        if (topLevel && GTK_IS_DIALOG(topLevel) &&
            !topProps->buttonOrderHacked) {
            // gtk_dialog_set_alternative_button_order will cause errors to be
            // logged, but dont want these so register our own error handler,
            // and then unregister afterwards...
            unsigned id = g_log_set_handler("Gtk", G_LOG_LEVEL_CRITICAL,
                                            gtkLogHandler, nullptr);
            topProps->buttonOrderHacked = true;

            gtk_dialog_set_alternative_button_order(
                GTK_DIALOG(topLevel), GTK_RESPONSE_HELP, GTK_RESPONSE_OK,
                GTK_RESPONSE_YES, GTK_RESPONSE_ACCEPT, GTK_RESPONSE_APPLY,
                GTK_RESPONSE_REJECT, GTK_RESPONSE_CLOSE, GTK_RESPONSE_NO,
                GTK_RESPONSE_CANCEL, -1);
            g_log_remove_handler("Gtk", id);
        }
    }

    if (opts.windowDrag > WM_DRAG_MENU_AND_TOOLBAR &&
        oneOf(detail, "base", "eventbox", "viewportbin")) {
        WMMove::setup(widget);
    }

    if (widget && ((100!=opts.bgndOpacity && GTK_IS_WINDOW(widget)) ||
                   (100!=opts.dlgOpacity && GTK_IS_DIALOG(widget))) &&
        !isFixedWidget(widget) && isRgbaWidget(widget)) {
        enableBlurBehind(widget, true);
    }

    if ((opts.menubarHiding || opts.statusbarHiding || BLEND_TITLEBAR ||
         opts.windowBorder & WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR) &&
        widget && GTK_IS_WINDOW(widget) && !isFixedWidget(widget) &&
        !isGimpDockable(widget) && !isMenuOrToolTipWindow) {
        if (Window::setup(widget, GTK_IS_DIALOG(widget) ? opts.dlgOpacity :
                          opts.bgndOpacity)) {
            GtkWidget *menuBar = Window::getMenuBar(widget, 0);
            GtkWidget *statusBar = (opts.statusbarHiding ?
                                    Window::getStatusBar(widget, 0) : nullptr);

            if (menuBar) {
                bool hiddenMenubar =
                    (opts.menubarHiding ?
                     qtcMenuBarHidden(qtSettings.appName) : false);
                QtcRect alloc = Widget::getAllocation(menuBar);

                if (hiddenMenubar)
                    gtk_widget_hide(menuBar);

                if (BLEND_TITLEBAR || opts.menubarHiding & HIDE_KWIN ||
                    opts.windowBorder &
                    WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR) {
                    Menu::emitSize(menuBar, hiddenMenubar ? 0 : alloc.height);
                }
                if (opts.menubarHiding&HIDE_KWIN) {
                    Window::menuBarDBus(widget,
                                        hiddenMenubar ? 0 : alloc.height);
                }
            }

#if GTK_CHECK_VERSION(2, 90, 0)
            if(gtk_window_get_has_resize_grip(GTK_WINDOW(widget)))
                gtk_window_set_has_resize_grip(GTK_WINDOW(widget), false);
#else
            if(statusBar && gtk_statusbar_get_has_resize_grip(GTK_STATUSBAR(statusBar)))
                gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusBar), false);
#endif

            if (opts.statusbarHiding && statusBar) {
                bool hiddenStatusBar = qtcStatusBarHidden(qtSettings.appName);
                if (hiddenStatusBar) {
                    gtk_widget_hide(statusBar);
                }
                if (opts.statusbarHiding & HIDE_KWIN) {
                    Window::statusBarDBus(widget, !hiddenStatusBar);
                    Window::setStatusBarProp(widget);
                }
            }
        }
    }

    if (widget && qtcIsCustomBgnd(opts) && oneOf(detail, "base", "eventbox")) {
        Scrollbar::setup(widget);
    }

    if (qtcIsCustomBgnd(opts) && DETAIL("viewportbin")) {
        GtkRcStyle *st = widget ? gtk_widget_get_modifier_style(widget) : nullptr;
        // if the app hasn't modified bg, draw background gradient
        if (st && !(st->color_flags[state]&GTK_RC_BG)) {
            drawWindowBgnd(cr, style, (QtcRect*)area, window, widget,
                           x, y, width, height);
            Scrollbar::setup(widget);
        } else {
            parent_class->draw_flat_box(style, window, state, shadow, area,
                                        widget, _detail, x, y, width, height);
        }
    } else if (qtcIsCustomBgnd(opts) && widget && GTK_IS_WINDOW(widget) &&
             !isMenuOrToolTipWindow &&
             drawWindowBgnd(cr, style, (QtcRect*)area, window, widget,
                            x, y, width, height)) {
        Window::setup(widget, GTK_IS_DIALOG(widget) ? opts.dlgOpacity :
                      opts.bgndOpacity);
    } else if(widget && GTK_IS_TREE_VIEW(widget)) {
        bool isCombo = isComboBoxPopupWindow(widget, 0);
        GtkTreeView *treeView = GTK_TREE_VIEW(widget);
        bool checkRules = (opts.forceAlternateLvCols ||
                           gtk_tree_view_get_rules_hint(treeView));
        bool isEven = checkRules && DETAILHAS("cell_even");

        if (qtSettings.app == GTK_APP_JAVA_SWT)
            area = nullptr;

        /* SWT seems to draw a 'cell_even', and then 'cell_odd' at the same position. This causes the view painting
         * to be messed up. Try and hack around this... */
        if (qtSettings.app == GTK_APP_JAVA_SWT && state == GTK_STATE_SELECTED &&
            checkRules && !isCombo && widget) {
            static GtkWidget *lastWidget = nullptr;
            static int lastEven = -1;

            if (DETAILHAS("cell_even")) {
                lastWidget = widget;
                lastEven = y;
            } else if (DETAILHAS("cell_odd")) {
                if (lastWidget == widget) {
                    if (y == lastEven) {
                        isEven = true;
                    }
                }
                lastWidget = nullptr;
                lastEven = -1;
            }
        }

        if (!isCombo || state != GTK_STATE_SELECTED) {
            Cairo::rect(cr, (QtcRect*)area, x, y, width, height,
                        getCellCol(haveAlternateListViewCol() &&
                                   checkRules && !isEven ?
                                   &qtSettings.colors[PAL_ACTIVE][COLOR_LV] :
                                   &style->base[GTK_STATE_NORMAL], detail));
        }
        if (isCombo) {
            if (state == GTK_STATE_SELECTED) {
                Cairo::rect(cr, (QtcRect*)area, x, y, width, height,
                            &style->base[widget &&
                                         gtk_widget_has_focus(widget) ?
                                         GTK_STATE_SELECTED :
                                         GTK_STATE_ACTIVE]);
            }
        } else {
            double alpha = 1.0;
            int selX = x;
            int selW = width;
            int factor = 0;
            bool forceCellStart = false;
            bool forceCellEnd = false;

            if (!isFixedWidget(widget)) {
                GtkTreePath *path = nullptr;
                GtkTreeViewColumn *column = nullptr;
                GtkTreeViewColumn *expanderColumn =
                    gtk_tree_view_get_expander_column(treeView);
                int levelIndent = 0;
                int expanderSize = 0;
                int depth = 0;

                TreeView::getCell(treeView, &path, &column,
                                  x, y, width, height);
                TreeView::setup(widget);
                if (path && TreeView::isCellHovered(widget, path, column)) {
                    if (state == GTK_STATE_SELECTED) {
                        factor = 10;
                    } else {
                        alpha = 0.2;
                    }
                }

                if (column == expanderColumn) {
                    gtk_widget_style_get(widget,
                                         "expander-size", &expanderSize, nullptr);
                    levelIndent = gtk_tree_view_get_level_indentation(treeView),
                    depth = path ? (int)gtk_tree_path_get_depth(path) : 0;

                    forceCellStart = true;
                    if (opts.lvLines) {
                        drawTreeViewLines(cr, &style->mid[GTK_STATE_ACTIVE],
                                          x, y, height, depth, levelIndent,
                                          expanderSize, treeView, path);
                    }
                } else if (column &&
                           TreeView::cellIsLeftOfExpanderColumn(treeView,
                                                                column)) {
                    forceCellEnd = true;
                }

                if ((state == GTK_STATE_SELECTED || alpha < 1.0) &&
                    column == expanderColumn) {
                    int offset = (3 + expanderSize * depth +
                                  (4 + levelIndent) * (depth - 1));
                    selX += offset;
                    selW -= offset;
                }

                if(path)
                    gtk_tree_path_free(path);
            }

            if (state == GTK_STATE_SELECTED || alpha < 1.0) {
                auto round = ROUNDED_NONE;
                if (opts.round != ROUND_NONE) {
                    if (forceCellStart && forceCellEnd) {
                        round = ROUNDED_ALL;
                    } else if (forceCellStart || strstr(detail, "_start")) {
                        round = ROUNDED_LEFT;
                    } else if (forceCellEnd || strstr(detail, "_end")) {
                        round = ROUNDED_RIGHT;
                    } else if (!strstr(detail, "_middle")) {
                        round = ROUNDED_ALL;
                    }
                }
                drawSelection(cr, style, state, (QtcRect*)area, widget, selX,
                              y, selW, height, round, true, alpha, factor);
            }
        }
    } else if(strcmp(detail, "checkbutton") == 0) {
        if (state == GTK_STATE_PRELIGHT && opts.crHighlight &&
            width > opts.crSize * 2) {
            GdkColor col=shadeColor(&style->bg[state], TO_FACTOR(opts.crHighlight));
            drawSelectionGradient(cr, (QtcRect*)area, x, y, width, height, ROUNDED_ALL, false, 1.0, &col, true);
        }
    } else if (strcmp(detail, "expander") == 0) {
        if (state == GTK_STATE_PRELIGHT && opts.expanderHighlight) {
            GdkColor col=shadeColor(&style->bg[state], TO_FACTOR(opts.expanderHighlight));
            drawSelectionGradient(cr, (QtcRect*)area, x, y, width, height, ROUNDED_ALL, false, 1.0, &col, true);
        }
    } else if (DETAIL("tooltip")) {
        drawToolTip(cr, widget, (QtcRect*)area, x, y, width, height);
    } else if (DETAIL("icon_view_item")) {
        drawSelection(cr, style, state, (QtcRect*)area, widget, x, y,
                      width, height, ROUNDED_ALL, false, 1.0, 0);
    } else if (state != GTK_STATE_SELECTED &&
               qtcIsCustomBgnd(opts) && DETAIL("eventbox")) {
        drawWindowBgnd(cr, style, nullptr, window, widget, x, y, width, height);
    } else if (!(qtSettings.app == GTK_APP_JAVA && widget &&
                 GTK_IS_LABEL(widget))) {
        if (state != GTK_STATE_PRELIGHT || opts.crHighlight ||
            strcmp(detail, "checkbutton") != 0) {
            parent_class->draw_flat_box(style, window, state, shadow, area,
                                        widget, _detail, x, y, width, height);
        }

        /* For SWT (e.g. eclipse) apps. For some reason these only seem to allow a ythickness of at max 2 - but
           for etching we need 3. So we fake this by drawing the 3rd lines here...*/

/*
        if(DO_EFFECT && GTK_STATE_INSENSITIVE!=state && DETAIL("entry_bg") &&
           isSwtComboBoxEntry(widget) && gtk_widget_has_focus(widget))
        {
            Cairo::hLine(cr, x, y, width,
                         &qtcPalette.highlight[FRAME_DARK_SHADOW]);
            Cairo::hLine(cr, x, y + height - 1, width,
                         &qtcPalette.highlight[0]);
        }
*/
    }
    cairo_destroy(cr);
}

static void
gtkDrawHandle(GtkStyle *style, GdkWindow *window, GtkStateType state,
              GtkShadowType shadow, GdkRectangle *_area, GtkWidget *widget,
              const char *_detail, int x, int y, int width, int height,
              GtkOrientation)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_WINDOW(window));
    const char *detail = _detail ? _detail : "";
    QtcRect *area = (QtcRect*)_area;
    bool paf = widgetIsType(widget, "PanelAppletFrame");
    cairo_t *cr = Cairo::gdkCreateClip(window, area);

    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %s  ", __FUNCTION__, state, shadow,
               width, height, _detail);
        debugDisplayWidget(widget, 10);
    }

    sanitizeSize(window, &width, &height);
    if (qtcIsFlatBgnd(opts.bgndAppearance) ||
        !(widget && drawWindowBgnd(cr, style, area, window, widget,
                                   x, y, width, height))) {
        if (widget && opts.bgndImage.type != IMG_NONE) {
            drawWindowBgnd(cr, style, area, window, widget,
                           x, y, width, height);
        }
    }

    if ((!strcmp(detail, "paned") || !strcmp(detail + 1, "paned"))) {
        drawSplitter(cr, state, style, area, x, y, width, height);
    } else if ((DETAIL("handlebox") &&
                (qtSettings.app == GTK_APP_JAVA ||
                 (widget && GTK_IS_HANDLE_BOX(widget)))) ||
               DETAIL("dockitem") || paf) {
        /* Note: I'm not sure why the 'widget && GTK_IS_HANDLE_BOX(widget)' is in
         * the following 'if' - its been there for a while. But this breaks the
         * toolbar handles for Java Swing apps. I'm leaving it in for non Java
         * apps, as there must've been a reason for it.... */
        if (widget && state != GTK_STATE_INSENSITIVE) {
            state = gtk_widget_get_state(widget);
        }
        if (paf) {
            /* The paf here is expected to be on the gnome panel */
            if (height < width) {
                y++;
            } else {
                x++;
            }
        } else {
            gtkDrawBox(style, window, state, shadow, (GdkRectangle*)area,
                       widget, "handlebox", x, y, width, height);
        }

        switch (opts.handles) {
        case LINE_1DOT:
            Cairo::dot(cr, x, y, width, height,
                       &qtcPalette.background[QTC_STD_BORDER]);
            break;
        case LINE_NONE:
            break;
        case LINE_DOTS:
            Cairo::dots(cr, x, y, width, height, height < width, 2, 5,
                        area, 2, &qtcPalette.background[5],
                        qtcPalette.background);
            break;
        case LINE_DASHES:
            if (height > width) {
                drawLines(cr, x + 3, y, 3, height, true, (height - 8) / 2, 0,
                          qtcPalette.background, area, 5, opts.handles);
            } else {
                drawLines(cr, x, y + 3, width, 3, false, (width - 8) / 2,
                          0, qtcPalette.background, area, 5, opts.handles);
            }
            break;
        case LINE_FLAT:
            drawLines(cr, x, y, width, height, height < width, 2, 4,
                      qtcPalette.background, area, 4, opts.handles);
            break;
        default:
            drawLines(cr, x, y, width, height, height < width, 2, 4,
                      qtcPalette.background, area, 3, opts.handles);
        }
    }
    cairo_destroy(cr);
}

static void
drawArrow(GdkWindow *window, const GdkColor *col, const QtcRect *area,
          GtkArrowType arrow_type, int x, int y, bool small, bool fill)
{
    cairo_t *cr = gdk_cairo_create(window);
    Cairo::arrow(cr, col, area, arrow_type, x, y, small, fill, opts.vArrows);
    cairo_destroy(cr);
}

static void
gtkDrawArrow(GtkStyle *style, GdkWindow *window, GtkStateType state,
             GtkShadowType shadow, GdkRectangle *_area, GtkWidget *widget,
             const char *_detail, GtkArrowType arrow_type, gboolean,
             int x, int y, int width, int height)
{
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %d %d %s  ", __FUNCTION__,
               state, shadow, arrow_type, x, y, width, height, _detail);
        debugDisplayWidget(widget, 10);
    }
    QtcRect *area = (QtcRect*)_area;
    cairo_t *cr = gdk_cairo_create(window);

    if (DETAIL("arrow")) {
        bool onComboEntry = isOnComboEntry(widget, 0);

        if (isOnComboBox(widget, 0) && !onComboEntry) {
            if (state == GTK_STATE_ACTIVE)
                state = GTK_STATE_PRELIGHT;
            const GdkColor *arrowColor =
                MO_ARROW(false,
                         &qtSettings.colors[state == GTK_STATE_INSENSITIVE ?
                                            PAL_DISABLED : PAL_ACTIVE]
                         [COLOR_BUTTON_TEXT]);
            x++;
            // NOTE: Dont do this for moz - as looks odd fir widgets in
            // HTML pages - arrow is shifted too much :-(
            if (!(opts.buttonEffect != EFFECT_NONE))
                x += 2;
            if (opts.doubleGtkComboArrow) {
                int pad = opts.vArrows ? 0 : 1;
                Cairo::arrow(cr, arrowColor, area, GTK_ARROW_UP, x + width / 2,
                             y + height / 2 - (LARGE_ARR_HEIGHT - pad),
                             false, true, opts.vArrows);
                Cairo::arrow(cr, arrowColor, area,  GTK_ARROW_DOWN,
                             x + width / 2,
                             y + height / 2 + (LARGE_ARR_HEIGHT - pad),
                             false, true, opts.vArrows);
            } else {
                GtkWidget *parent = nullptr;
                if (!opts.gtkComboMenus &&
                    !((parent = gtk_widget_get_parent(widget)) &&
                      (parent = gtk_widget_get_parent(parent)) &&
                      !ComboBox::hasFrame(parent))) {
                    x += 2;
                }
                Cairo::arrow(cr, arrowColor, area,  GTK_ARROW_DOWN,
                             x + width / 2, y + height / 2, false, true,
                             opts.vArrows);
            }
        } else {
            bool combo = onComboEntry || isOnCombo(widget, 0);
            int origState = state;

            if (combo && state == GTK_STATE_ACTIVE)
                state = GTK_STATE_PRELIGHT;

            const GdkColor *col =
                (combo || isOnListViewHeader(widget, 0) ||
                 isOnButton(widget, 0, nullptr) ?
                 &qtSettings.colors[state == GTK_STATE_INSENSITIVE ?
                                    PAL_DISABLED :
                                    PAL_ACTIVE][COLOR_BUTTON_TEXT] :
                 &style->text[ARROW_STATE(state)]);
            if (onComboEntry && origState == GTK_STATE_ACTIVE &&
                opts.unifyCombo) {
                x--;
                y--;
            }
            Cairo::arrow(cr, MO_ARROW(false, col), area, arrow_type,
                         x + width / 2, y + height / 2,
                         false, true, opts.vArrows);
        }
    } else {
        int isSpinButton = DETAIL("spinbutton");
        bool isMenuItem = DETAIL("menuitem");
        /* int a_width = LARGE_ARR_WIDTH; */
        /* int a_height = LARGE_ARR_HEIGHT; */
        bool sbar = isSbarDetail(detail);
        bool smallArrows = isSpinButton && !opts.unifySpin;
        int stepper = (sbar ? getStepper(widget, x, y, opts.sliderWidth,
                                         opts.sliderWidth) : STEPPER_NONE);

        sanitizeSize(window, &width, &height);

        if (isSpinButton)  {
            /* if (GTK_ARROW_UP == arrow_type) */
            /*     y++; */
            /* a_height = SMALL_ARR_HEIGHT; */
            /* a_width = SMALL_ARR_WIDTH; */
        } else if (oneOf(arrow_type, GTK_ARROW_LEFT, GTK_ARROW_RIGHT) ||
                   isMenuItem) {
            /* a_width = LARGE_ARR_HEIGHT; */
            /* a_height = LARGE_ARR_WIDTH; */
            if (isMozilla() && opts.vArrows /* && a_height */ &&
                height < LARGE_ARR_WIDTH) {
                smallArrows = true;
            }
        }
        x += width / 2;
        y += height / 2;
        if (state == GTK_STATE_ACTIVE && ((sbar && !opts.flatSbarButtons) ||
                                          (isSpinButton && !opts.unifySpin))) {
            x++;
            y++;
        }
        if (sbar) {
            switch (stepper) {
            case STEPPER_B:
                if (opts.flatSbarButtons || !opts.vArrows) {
                    if (GTK_ARROW_RIGHT == arrow_type) {
                        x--;
                    } else {
                        y--;
                    }
                }
                break;
            case STEPPER_C:
                if (opts.flatSbarButtons || !opts.vArrows) {
                    if (GTK_ARROW_LEFT == arrow_type) {
                        x++;
                    } else {
                        y++;
                    }
                }
            default:
                break;
            }
        }

        if (isSpinButton && isFixedWidget(widget) && isFakeGtk()) {
            x--;
        }
        if (isSpinButton && !(opts.buttonEffect != EFFECT_NONE)) {
            y += arrow_type == GTK_ARROW_UP ? -1 : 1;
        }
        if (opts.unifySpin && isSpinButton && !opts.vArrows &&
            arrow_type == GTK_ARROW_DOWN) {
            y--;
        }
        if (state == GTK_STATE_ACTIVE && (sbar  || isSpinButton) &&
            opts.coloredMouseOver == MO_GLOW) {
            state = GTK_STATE_PRELIGHT;
        }
        if (isMenuItem && arrow_type == GTK_ARROW_RIGHT && !isFakeGtk()) {
            x -= 2;
        }
        const GdkColor *col =
            (isSpinButton || sbar ?
             &qtSettings.colors[state == GTK_STATE_INSENSITIVE ?
                                PAL_DISABLED : PAL_ACTIVE][COLOR_BUTTON_TEXT] :
             &style->text[isMenuItem && state == GTK_STATE_PRELIGHT ?
                          GTK_STATE_SELECTED : ARROW_STATE(state)]);
        if (isMenuItem && state != GTK_STATE_PRELIGHT && opts.shadePopupMenu) {
            if (opts.shadeMenubars == SHADE_WINDOW_BORDER) {
                col = &qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER_TEXT];
            } else if (opts.customMenuTextColor) {
                col = &opts.customMenuNormTextColor;
            } else if (oneOf(opts.shadeMenubars, SHADE_BLEND_SELECTED,
                             SHADE_SELECTED) ||
                        (opts.shadeMenubars == SHADE_CUSTOM &&
                         TOO_DARK(qtcPalette.menubar[ORIGINAL_SHADE]))) {
                col = &style->text[GTK_STATE_SELECTED];
            }
        }
        Cairo::arrow(cr, MO_ARROW(isMenuItem, col), area, arrow_type, x, y,
                     smallArrows, true, opts.vArrows);
    }
    cairo_destroy(cr);
}

static void
drawBox(GtkStyle *style, GdkWindow *window, GtkStateType state,
        GtkShadowType shadow, GdkRectangle *area, GtkWidget *widget,
        const char *_detail, int x, int y, int width, int height,
        bool btnDown)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    bool sbar = isSbarDetail(detail);
    bool pbar = DETAIL("bar"); //  && GTK_IS_PROGRESS_BAR(widget);
    bool qtcSlider = !pbar && DETAIL("qtc-slider");
    bool slider = qtcSlider || (!pbar && DETAIL("slider"));
    bool hscale = !slider && DETAIL("hscale");
    bool vscale = !hscale && DETAIL("vscale");
    bool menubar = !vscale && DETAIL("menubar");
    bool button = !menubar && DETAIL("button");
    bool togglebutton = !button && DETAIL("togglebutton");
    bool optionmenu = !togglebutton && DETAIL("optionmenu");
    bool stepper = !optionmenu && DETAIL("stepper");
    bool vscrollbar = (!optionmenu && qtcStrStartsWith(detail, "vscrollbar"));
    bool hscrollbar = (!vscrollbar && qtcStrStartsWith(detail, "hscrollbar"));
    bool spinUp = !hscrollbar && DETAIL("spinbutton_up");
    bool spinDown = !spinUp && DETAIL("spinbutton_down");
    bool menuScroll = strstr(detail, "menu_scroll_arrow_");
    bool rev = (reverseLayout(widget) ||
                (widget && reverseLayout(gtk_widget_get_parent(widget))));
    bool activeWindow = true;
    GdkColor new_cols[TOTAL_SHADES + 1];
    const GdkColor *btnColors = qtcPalette.background;
    int bgnd = getFill(state, btnDown);
    auto round = getRound(detail, widget, rev);
    bool lvh = (isListViewHeader(widget) ||
                isEvolutionListViewHeader(widget, detail));
    bool sunken = (btnDown || shadow == GTK_SHADOW_IN ||
                   state == GTK_STATE_ACTIVE || bgnd == 2 || bgnd == 3);
    GtkWidget *parent = nullptr;

    if (button && GTK_IS_TOGGLE_BUTTON(widget)) {
        button = false;
        togglebutton = true;
    }

    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %d %d %s  ", __FUNCTION__,
               btnDown, state, shadow, x, y, width, height, _detail);
        debugDisplayWidget(widget, 10);
    }

    // FIXME, need to update useButtonColor if the logic below changes right now
    if (useButtonColor(detail)) {
        if (slider | hscale | vscale | sbar && state == GTK_STATE_INSENSITIVE) {
            btnColors = qtcPalette.background;
        } else if (QT_CUSTOM_COLOR_BUTTON(style)) {
            shadeColors(&style->bg[state], new_cols);
            btnColors = new_cols;
        } else {
            SET_BTN_COLS(slider, hscale | vscale, lvh, state);
        }
    }

    if (menubar && !isFakeGtk() && opts.shadeMenubarOnlyWhenActive) {
        GtkWindow *topLevel = GTK_WINDOW(gtk_widget_get_toplevel(widget));

        if (topLevel && GTK_IS_WINDOW(topLevel)) {
            GtkWidgetProps topProps(topLevel);
            if (!topProps->shadeActiveMBHacked) {
                topProps->shadeActiveMBHacked = true;
                g_signal_connect(G_OBJECT(topLevel), "event",
                                 G_CALLBACK(windowEvent), widget);
            }
            activeWindow = Window::isActive(GTK_WIDGET(topLevel));
        }
    }

    if (opts.menubarMouseOver && GTK_IS_MENU_SHELL(widget) && !isFakeGtk()) {
        Menu::shellSetup(widget);
    }

    cairo_t *cr = Cairo::gdkCreateClip(window, area);
    if (spinUp || spinDown) {
        if (!opts.unifySpin && (!opts.unifySpinBtns || sunken)) {
            EWidget wid = spinUp ? WIDGET_SPIN_UP : WIDGET_SPIN_DOWN;
            QtcRect *a = (QtcRect*)area;
            QtcRect b;
            QtcRect unified;
            bool ooOrMoz = isFakeGtk();
            if (!a && isFixedWidget(widget) && ooOrMoz) {
                b = qtcRect(x, y, width, height);
                a = &b;
            }
            if (wid == WIDGET_SPIN_UP) {
                if (opts.buttonEffect != EFFECT_NONE && opts.etchEntry) {
                    if (!opts.unifySpinBtns)
                        drawEtch(cr, a, widget, x - 2, y, width + 2, height * 2,
                                 false, ROUNDED_RIGHT, WIDGET_SPIN_UP);
                    y++;
                    width--;
                }
                height++;

                if (opts.unifySpinBtns) {
                    unified = qtcRect(x, y, width,
                                      height - (state == GTK_STATE_PRELIGHT ?
                                                2 : 1));
                    height *= 2;
                    area = (GdkRectangle*)&unified;
                } else if (!opts.etchEntry) {
                    height++;
                }
            } else if (opts.buttonEffect != EFFECT_NONE && opts.etchEntry) {
                QtcRect clip = {x - 2, y, width + 2, height};
                if (!opts.unifySpinBtns) {
                    drawEtch(cr, ooOrMoz ? a : &clip, widget, x - 2, y - 2,
                             width + 2, height + 2, false,
                             ROUNDED_RIGHT, WIDGET_SPIN_DOWN);
                }
                height--;
                width--;
                if (opts.unifySpinBtns) {
                    unified = qtcRect(
                        x, y + (state == GTK_STATE_PRELIGHT ? 1 : 0),
                        width, height - (state == GTK_STATE_PRELIGHT ? 1 : 0));
                    y -= height;
                    height *= 2;
                    area = (GdkRectangle*)&unified;
                }
            }

            drawBgnd(cr, &btnColors[bgnd], widget, (QtcRect*)area,
                     x + 1, y + 1, width - 2, height - 2);
            drawLightBevel(cr, style, state, (QtcRect*)area, x, y, width,
                           height - (WIDGET_SPIN_UP == wid &&
                                     opts.buttonEffect != EFFECT_NONE ? 1 : 0),
                           &btnColors[bgnd], btnColors, round, wid, BORDER_FLAT,
                           DF_DO_BORDER | (sunken ? DF_SUNKEN : 0), widget);
        }
    } else if (DETAIL("spinbutton")) {
        if (qtcIsFlatBgnd(opts.bgndAppearance) ||
            !(widget && drawWindowBgnd(cr, style, (QtcRect*)area, window,
                                       widget, x, y, width, height))) {
            gtk_style_apply_default_background(
                style, window, widget && gtk_widget_get_has_window(widget),
                state == GTK_STATE_INSENSITIVE ? GTK_STATE_INSENSITIVE :
                GTK_STATE_NORMAL, area, x, y, width, height);
            if (widget && opts.bgndImage.type != IMG_NONE) {
                drawWindowBgnd(cr, style, (QtcRect*)area, window, widget,
                               x, y, width, height);
            }
        }

        if (opts.unifySpin) {
            bool rev = (reverseLayout(widget) ||
                        (widget &&
                         reverseLayout(gtk_widget_get_parent(widget))));
            bool moz = isMozillaWidget(widget);
            if (!rev) {
                x -= 4;
            }
            width += 4;

            Cairo::Saver saver(cr);
            if (moz) {
                QtcRect a = {x + 2, y, width - 2, height};
                Cairo::clipRect(cr, &a);
            }
            drawEntryField(cr, style, state, window, widget, (QtcRect*)area,
                           x, y, width, height,
                           rev ? ROUNDED_LEFT : ROUNDED_RIGHT, WIDGET_SPIN);
        } else if (opts.unifySpinBtns) {
            int offset = (opts.buttonEffect != EFFECT_NONE && opts.etchEntry ?
                          1 : 0);
            if (offset) {
                drawEtch(cr, (QtcRect*)area, widget, x, y, width, height, false,
                         ROUNDED_RIGHT, WIDGET_SPIN);
            }
#if GTK_CHECK_VERSION(2, 90, 0)
            bgnd = getFill(state == GTK_STATE_ACTIVE ? GTK_STATE_NORMAL : state,
                           false);
#endif
            drawLightBevel(cr, style, state, (QtcRect*)area, x, y + offset,
                           width - offset, height - 2 * offset,
                           &btnColors[bgnd], btnColors, ROUNDED_RIGHT,
                           WIDGET_SPIN, BORDER_FLAT,
                           DF_DO_BORDER | (sunken ? DF_SUNKEN : 0), widget);
            drawFadedLine(cr, x + 2, y + height / 2, width - (offset + 4), 1,
                          &btnColors[QTC_STD_BORDER], (QtcRect*)area, nullptr,
                          true, true, true);
        }
    } else if (!opts.stdSidebarButtons && (button || togglebutton) &&
               isSideBarBtn(widget)) {
        drawSidebarButton(cr, state, style, (QtcRect*)area,
                          x, y, width, height);
    } else if (lvh) {
        if (opts.highlightScrollViews && widget) {
            GtkWidget *parent = gtk_widget_get_parent(widget);
            if (parent && GTK_IS_TREE_VIEW(parent)) {
                ScrolledWindow::registerChild(parent);
            }
        }
        drawListViewHeader(cr, state, btnColors, bgnd, (QtcRect*)area,
                           x, y, width, height);
    } else if (isPathButton(widget)) {
        if (state == GTK_STATE_PRELIGHT) {
            drawSelection(cr, style, state, (QtcRect*)area, widget,
                          x, y, width, height, ROUNDED_ALL, false, 1.0, 0);
        }
        if (opts.windowDrag > WM_DRAG_MENU_AND_TOOLBAR) {
            WMMove::setup(widget);
        }
        if (GTK_IS_TOGGLE_BUTTON(widget)) {
            drawArrow(window, &qtcPalette.background[5], (QtcRect*)area,
                      GTK_ARROW_RIGHT, x + width - (LARGE_ARR_WIDTH / 2 + 4),
                      y + (height - LARGE_ARR_HEIGHT / 2) / 2 + 1, false, true);
        }
    } else if (button || togglebutton || optionmenu || sbar ||
               hscale || vscale || stepper || slider) {
        bool combo = (strcmp(detail, "optionmenu") == 0 ||
                      isOnComboBox(widget, 0));
        bool combo_entry = combo && isOnComboEntry(widget, 0);
        bool horiz_tbar;
        bool tbar_button = isButtonOnToolbar(widget, &horiz_tbar);
        bool handle_button = (!tbar_button &&
                              isButtonOnHandlebox(widget, &horiz_tbar));
        int xAdjust = 0;
        int yAdjust = 0;
        int wAdjust = 0;
        int hAdjust = 0;
        bool horiz = ((tbar_button || handle_button) &&
                      IS_GLASS(opts.appearance) &&
                      IS_GLASS(opts.toolbarAppearance) ? horiz_tbar :
                      (slider && width < height) || vscrollbar || vscale ||
                      (stepper && widget && GTK_IS_VSCROLLBAR(widget)) ?
                      false : true);
        bool defBtn = (state != GTK_STATE_INSENSITIVE &&
                       (button || togglebutton) && widget &&
                       gtk_widget_has_default(widget));
        if (combo && !sunken && isActiveOptionMenu(widget)) {
            sunken = true;
            bgnd = 4;
        }
        if (tbar_button && opts.tbarBtns == TBTN_JOINED) {
            adjustToolbarButtons(widget, &xAdjust, &yAdjust, &wAdjust, &hAdjust,
                                 &round, horiz_tbar);
            x += xAdjust;
            y += yAdjust;
            width += wAdjust;
            height += hAdjust;
        }

        {
            /* Yuck this is a horrible mess!!!!! */
            bool glowFocus = (widget && gtk_widget_has_focus(widget) &&
                              opts.coloredMouseOver == MO_GLOW &&
                              oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED));
            EWidget widgetType=isComboBoxButton(widget)
                                ? WIDGET_COMBO_BUTTON
                                : slider
                                    ? qtcSlider ? WIDGET_SLIDER : WIDGET_SB_SLIDER
                                    : hscale||vscale
                                        ? WIDGET_SLIDER
                                        : combo || optionmenu
                                            ? WIDGET_COMBO
                                            : tbar_button
                                                ? (opts.coloredTbarMo ? WIDGET_TOOLBAR_BUTTON : WIDGET_UNCOLOURED_MO_BUTTON)
                                                : togglebutton
                                                    ? (glowFocus && !sunken ? WIDGET_DEF_BUTTON : WIDGET_TOGGLE_BUTTON)
                                                    : button
                                                        ? defBtn || glowFocus
                                                            ? WIDGET_DEF_BUTTON
                                                            : WIDGET_STD_BUTTON
                                                        : stepper || sbar
                                                            ? WIDGET_SB_BUTTON
                                                            : WIDGET_OTHER;
            int xo=x, yo=y, wo=width, ho=height, stepper=STEPPER_NONE;

            /* Try and guess if this button is a toolbar button... */
            if (oneOf(widgetType, WIDGET_STD_BUTTON, WIDGET_TOGGLE_BUTTON) &&
                isMozillaWidget(widget) && GTK_IS_BUTTON(widget) &&
                DETAIL("button") && ((width > 22 && width < 56 &&
                                      height > 30) || height >= 32 ||
                                     ((width == 30 || width == 45) &&
                                      height == 30)))
                widgetType = (opts.coloredTbarMo ? WIDGET_TOOLBAR_BUTTON :
                              WIDGET_UNCOLOURED_MO_BUTTON);

            if (ROUND_MAX==opts.round &&
                ((WIDGET_TOGGLE_BUTTON==widgetType && height>(opts.crSize+8) && width<(height+10)) ||
                 (GTK_APP_GIMP==qtSettings.app && WIDGET_STD_BUTTON==widgetType && widgetIsType(widget, "GimpViewableButton")) ||
                    (opts.stdSidebarButtons && WIDGET_STD_BUTTON==widgetType && widget && isSideBarBtn(widget)) ||
                    (WIDGET_STD_BUTTON==widgetType && GTK_APP_OPEN_OFFICE==qtSettings.app && isFixedWidget(widget) &&
                    height>30 && height<40 && width>16 && width<50) ) )
                widgetType=WIDGET_TOOLBAR_BUTTON;

            /* For some reason SWT combo's dont un-prelight when activated! So dont pre-light at all! */
/*
            if(GTK_APP_JAVA_SWT==qtSettings.app && WIDGET_STD_BUTTON==widgetType && GTK_STATE_PRELIGHT==state && WIDGET_COMBO==widgetType)
            {
                state=GTK_STATE_NORMAL;
                bgnd=getFill(state, btnDown);
            }
            else */ if(WIDGET_SB_BUTTON==widgetType && GTK_APP_MOZILLA!=qtSettings.app)
            {
                stepper = getStepper(widget, x, y, width, height);
                switch (stepper) {
                case STEPPER_B:
                    if (horiz) {
                        x--;
                        width++;
                    } else {
                        y--;
                        height++;
                    }
                    break;
                case STEPPER_C:
                    if (horiz) {
                        width++;
                    } else {
                        height++;
                    }
                default:
                    break;
                }
            }

            if(/*GTK_APP_JAVA_SWT==qtSettings.app && */
                widget && !isFixedWidget(widget) && /* Don't do for Firefox, etc. */
                WIDGET_SB_SLIDER==widgetType && GTK_STATE_INSENSITIVE!=state && GTK_IS_RANGE(widget))
            {
                QtcRect alloc = Widget::getAllocation(widget);
                bool horizontal = !Widget::isHorizontal(widget);
                int sbarTroughLen = (horizontal ? alloc.height : alloc.width) -
                    ((qtcRangeHasStepperA(widget) ? opts.sliderWidth : 0) +
                     (qtcRangeHasStepperB(widget) ? opts.sliderWidth : 0) +
                     (qtcRangeHasStepperC(widget) ? opts.sliderWidth : 0) +
                     (qtcRangeHasStepperD(widget) ? opts.sliderWidth : 0));
                int sliderLen = horizontal ? height : width;

                if(sbarTroughLen==sliderLen)
                {
                    state=GTK_STATE_INSENSITIVE;
                    btnColors=qtcPalette.background;
                    bgnd=getFill(state, false);
                }
            }
#ifdef INCREASE_SB_SLIDER
            if(slider && widget && GTK_IS_RANGE(widget) && !opts.flatSbarButtons && SCROLLBAR_NONE!=opts.scrollbarType
                /*&& !(GTK_STATE_PRELIGHT==state && MO_GLOW==opts.coloredMouseOver)*/)
            {
                GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));
                bool horizontal = Widget::isHorizontal(widget);
#if GTK_CHECK_VERSION(2, 90, 0)
                bool hasStartStepper = SCROLLBAR_PLATINUM!=opts.scrollbarType;
                bool hasEndStepper = SCROLLBAR_NEXT!=opts.scrollbarType;
#else
                bool hasStartStepper = (qtcRangeHasStepperA(widget) ||
                                        qtcRangeHasStepperB(widget));
                bool hasEndStepper = (qtcRangeHasStepperC(widget) ||
                                      qtcRangeHasStepperD(widget));
#endif
                bool atEnd = false;
                double value = gtk_adjustment_get_value(adj);

                if (hasStartStepper && value <= gtk_adjustment_get_lower(adj)) {
                    if (horizontal) {
                        x--;
                        width++;
                    } else {
                        y--;
                        height++;
                    }
                    atEnd = true;
                }
                if (hasEndStepper &&
                    value >= (gtk_adjustment_get_upper(adj) -
                              gtk_adjustment_get_page_size(adj))) {
                    if (horizontal) {
                        width++;
                    } else {
                        height++;
                    }
                    atEnd = true;
                }
                if (!isMozilla() && widget &&
                    lastSlider.widget == widget && !atEnd) {
                    lastSlider.widget = nullptr;
                }
            }
#endif

#if !GTK_CHECK_VERSION(2, 90, 0) /* Gtk3:TODO !!! */
            if(GTK_APP_OPEN_OFFICE==qtSettings.app && opts.flatSbarButtons && slider &&
                (SCROLLBAR_KDE==opts.scrollbarType || SCROLLBAR_WINDOWS==opts.scrollbarType) &&
                widget && GTK_IS_RANGE(widget) && isFixedWidget(widget)) {
                if (!Widget::isHorizontal(widget)) {
                    y++;
                    height--;
                } else {
                    x += 2;
                    width -= 2;
                }
            }
#endif
            if(WIDGET_COMBO==widgetType && !opts.gtkComboMenus && !isMozilla() &&
                ((parent=gtk_widget_get_parent(widget)) && GTK_IS_COMBO_BOX(parent) && !QTC_COMBO_ENTRY(parent)))
            {
                GtkWidget *mapped = nullptr;
                bool changedFocus = false;
                bool draw = true;
                int mod = 7;

                if (!opts.gtkComboMenus && !ComboBox::hasFrame(parent)) {
                    mod = 0;
                    draw = oneOf(state, GTK_STATE_ACTIVE, GTK_STATE_PRELIGHT);
                    ComboBox::setup(nullptr, parent);
                } else {
                    changedFocus = ComboBox::isFocusChanged(widget);
                    mapped = WidgetMap::getWidget(parent, 1);
                    WidgetMap::setup(parent, widget, 0);

                    if (parent && ComboBox::isHovered(parent)) {
                        state = GTK_STATE_PRELIGHT;
                    }
                }

                if (draw)
                    drawLightBevel(cr, style, state, (QtcRect*)area, x - mod, y,
                                   width + mod, height, &btnColors[bgnd],
                                   btnColors, round, WIDGET_TOOLBAR_BUTTON,
                                   BORDER_FLAT, (state == GTK_STATE_ACTIVE ?
                                                 DF_SUNKEN : 0) | DF_DO_BORDER,
                                   widget);

                if(mapped)
                {
                    if(changedFocus)
                        gtk_widget_queue_draw(mapped);
                    else
                    {
                        GtkStateType mappedState=gtk_widget_get_state(mapped);
                        if(GTK_STATE_INSENSITIVE==state && GTK_STATE_INSENSITIVE!=mappedState)
                            state=mappedState;
                        if(mappedState!=gtk_widget_get_state(widget) && GTK_STATE_INSENSITIVE!=mappedState && GTK_STATE_INSENSITIVE!=state)
                            gtk_widget_set_state(mapped, state);
                    }
                }
            }
            else if(opts.unifyCombo && WIDGET_COMBO_BUTTON==widgetType)
            {
                GtkWidget    *parent=widget ? gtk_widget_get_parent(widget) : nullptr;
                GtkWidget    *entry=parent ? getComboEntry(parent) : nullptr;
                GtkStateType entryState=entry ? gtk_widget_get_state(entry) : GTK_STATE_NORMAL;
                bool rev = false;
                bool mozToolbar = (isMozilla() && parent &&
                                   GTK_IS_TOGGLE_BUTTON(widget) &&
                                   QTC_COMBO_ENTRY(parent) &&
                                   (parent = gtk_widget_get_parent(parent)) &&
                                   GTK_IS_FIXED(parent) &&
                                   (parent = gtk_widget_get_parent(parent)) &&
                                   GTK_IS_WINDOW(parent) &&
                                   strcmp(gtk_widget_get_name(parent),
                                          "MozillaGtkWidget") == 0);

                if (!entry && widget && gtk_widget_get_parent(widget)) {
                    entry = WidgetMap::getWidget(
                        gtk_widget_get_parent(widget), 1);
                }

                if(entry)
                    rev=reverseLayout(entry);

                if(!rev)
                    x-=4;
                width+=4;
                if((mozToolbar && state==GTK_STATE_PRELIGHT) || state==GTK_STATE_ACTIVE)
                    state=GTK_STATE_NORMAL;

                // When we draw the entry, if its highlighted we want to highlight this button as well.
                // Unfortunately, when the entry of a GtkComboBoxEntry draws itself, there is no way to
                // determine the button associated with it. So, we store the mapping here...
                if(!mozToolbar && parent && QTC_COMBO_ENTRY(parent))
                    WidgetMap::setup(parent, widget, 0);
                // If the button is disabled, but the entry field is not - then use entry field's state
                // for the button. This fixes an issue with LinuxDC++ and Gtk 2.18
                if(GTK_STATE_INSENSITIVE==state && entry && GTK_STATE_INSENSITIVE!=entryState)
                    state=entryState;

                drawEntryField(cr, style, state, window, entry, (QtcRect*)area, x, y, width, height, rev ? ROUNDED_LEFT : ROUNDED_RIGHT, WIDGET_COMBO_BUTTON);

                // Get entry to redraw by setting its state...
                // ...cant do a queue redraw, as then entry does for the button, else we get stuck in a loop!
                if(!mozToolbar && widget && entry && entryState!=gtk_widget_get_state(widget) && GTK_STATE_INSENSITIVE!=entryState &&
                    GTK_STATE_INSENSITIVE!=state)
                    gtk_widget_set_state(entry, state);
            } else if (opts.flatSbarButtons && WIDGET_SB_BUTTON==widgetType) {
#if !GTK_CHECK_VERSION(2, 90, 0)
                if (isMozilla()) {
                    /* This section messes up Gtk3 scrollbars with custom background - and doesnt seem to be required for Gtk2 either. remove at 1.7.1 */
                    /* Re-added in 1.7.2 as needed by Mozilla! */
                    if(opts.gtkScrollViews && qtcIsFlat(opts.sbarBgndAppearance) && 0!=opts.tabBgnd && widget && gtk_widget_get_parent(widget) && gtk_widget_get_parent(widget)->parent &&
                       GTK_IS_SCROLLED_WINDOW(gtk_widget_get_parent(widget)) && GTK_IS_NOTEBOOK(gtk_widget_get_parent(widget)->parent))
                        drawAreaModColor(cr, (QtcRect*)area, &qtcPalette.background[ORIGINAL_SHADE], TO_FACTOR(opts.tabBgnd), xo, yo, wo, ho);
                    else if(qtcIsFlatBgnd(opts.bgndAppearance) || !(opts.gtkScrollViews && qtcIsFlat(opts.sbarBgndAppearance) &&
                                                              widget && drawWindowBgnd(cr, style, (QtcRect*)area, window, widget, xo, yo, wo, ho)))
                    {
                        if (!qtcIsFlat(opts.sbarBgndAppearance) &&
                            opts.scrollbarType != SCROLLBAR_NONE) {
                            drawBevelGradient(
                                cr, (QtcRect*)area, xo, yo, wo, ho,
                                &qtcPalette.background[ORIGINAL_SHADE], horiz,
                                false, opts.sbarBgndAppearance, WIDGET_SB_BGND);
                        } else {
                             drawBgnd(
                                 cr, &qtcPalette.background[ORIGINAL_SHADE],
                                 widget, (QtcRect*)area, xo, yo, wo, ho);
                        }
                    }
                }
#endif
            } else {
                const GdkColor *cols =
                    (defBtn && oneOf(opts.defBtnIndicator, IND_TINT,
                                     IND_COLORED, IND_SELECTED) ?
                     qtcPalette.defbtn : widgetType == WIDGET_COMBO_BUTTON &&
                     qtcPalette.combobtn && state != GTK_STATE_INSENSITIVE ?
                     qtcPalette.combobtn : btnColors);
                int bg = (WIDGET_COMBO_BUTTON==widgetType &&
                          (SHADE_DARKEN==opts.comboBtn ||
                           (SHADE_NONE != opts.comboBtn && GTK_STATE_INSENSITIVE==state))) ||
                            (WIDGET_SB_SLIDER==widgetType && SHADE_DARKEN==opts.shadeSliders) ||
                            (defBtn && IND_DARKEN==opts.defBtnIndicator)
                                ? getFill(state, btnDown, true) : bgnd;

                drawLightBevel(cr, style, state, (QtcRect*)area, x, y, width,
                               height, &cols[bg], cols, round, widgetType,
                               BORDER_FLAT, (sunken ? DF_SUNKEN : 0) |
                               DF_DO_BORDER | (horiz ? 0 : DF_VERT), widget);

                if(tbar_button && TBTN_JOINED==opts.tbarBtns)
                {
                    const int constSpace=4;
                    int xo=x-xAdjust, yo=y-yAdjust, wo=width-wAdjust, ho=height-hAdjust;

                    if(xAdjust)
                        drawFadedLine(cr, xo, yo + constSpace, 1,
                                      ho - 2 * constSpace, &btnColors[0],
                                      (QtcRect*)area, nullptr, true, true, false);
                    if (yAdjust)
                        drawFadedLine(cr, xo + constSpace, yo,
                                      wo - 2 * constSpace, 1, &btnColors[0],
                                      (QtcRect*)area, nullptr, true, true, true);
                    if(wAdjust && ROUNDED_RIGHT!=round)
                        drawFadedLine(cr, xo + wo - 1, yo + constSpace, 1,
                                      ho - 2 * constSpace,
                                      &btnColors[QTC_STD_BORDER],
                                      (QtcRect*)area, nullptr, true, true, false);
                    if (hAdjust && ROUNDED_BOTTOM!=round)
                        drawFadedLine(cr, xo + constSpace, yo + ho - 1,
                                      wo - 2 * constSpace, 1,
                                      &btnColors[QTC_STD_BORDER],
                                      (QtcRect*)area, nullptr, true, true, true);
                }
            }

#ifdef INCREASE_SB_SLIDER
            /* Gtk draws slider first, and then the buttons. But if we have a shaded slider, and extend this so that it
                overlaps (by 1 pixel) the buttons, then the top/bottom is vut off if this is shaded...
                So, work-around this by re-drawing the slider here! */
            if(!opts.flatSbarButtons && SHADE_NONE!=opts.shadeSliders && SCROLLBAR_NONE!=opts.scrollbarType &&
                WIDGET_SB_BUTTON==widgetType && widget && widget==lastSlider.widget && !isMozilla() &&
                ( (SCROLLBAR_NEXT==opts.scrollbarType && STEPPER_B==stepper) || STEPPER_D==stepper))
            {
#if GTK_CHECK_VERSION(2, 90, 0)
                gtkDrawSlider(lastSlider.style, lastSlider.cr, lastSlider.state, lastSlider.shadow, lastSlider.widget,
                                lastSlider.detail, lastSlider.x, lastSlider.y, lastSlider.width, lastSlider.height, lastSlider.orientation);
#else
                gtkDrawSlider(lastSlider.style, lastSlider.window, lastSlider.state, lastSlider.shadow, nullptr, lastSlider.widget,
                                lastSlider.detail, lastSlider.x, lastSlider.y, lastSlider.width, lastSlider.height, lastSlider.orientation);
#endif
                lastSlider.widget=nullptr;
            }
#endif
        }
        if (defBtn) {
            drawDefBtnIndicator(cr, state, btnColors, bgnd, sunken,
                                (QtcRect*)area, x, y, width, height);
        }
        if (opts.comboSplitter || opts.comboBtn != SHADE_NONE) {
            if (optionmenu) {
                GtkRequisition indicator_size;
                GtkBorder      indicator_spacing;
                int            cx=x, cy=y, cheight=height, cwidth=width,
                               ind_width=0,
                               darkLine=BORDER_VAL(GTK_STATE_INSENSITIVE!=state);

                optionMenuGetProps(widget, &indicator_size, &indicator_spacing);

                ind_width=indicator_size.width+indicator_spacing.left+indicator_spacing.right;

                if (opts.buttonEffect != EFFECT_NONE)
                    cx--;

                cy+=3;
                cheight-=6;

                if (opts.comboBtn != SHADE_NONE) {
                    const GdkColor *cols = (qtcPalette.combobtn &&
                                            state != GTK_STATE_INSENSITIVE ?
                                            qtcPalette.combobtn : btnColors);
                    int bg = SHADE_DARKEN==opts.comboBtn || (GTK_STATE_INSENSITIVE==state && SHADE_NONE!=opts.comboBtn) ? getFill(state, btnDown, true) : bgnd;

                    QtcRect btn = {
                        cx + (rev ? ind_width + style->xthickness :
                              (cwidth - ind_width - style->xthickness) + 1),
                        y, ind_width + 3, height
                    };
                    Cairo::Saver saver(cr);
                    if (!opts.comboSplitter) {
                        Cairo::clipRect(cr, &btn);
                    }
                    if (rev) {
                        btn.width += 3;
                    } else {
                        btn.x -= 3;
                        if (opts.buttonEffect != EFFECT_NONE) {
                            btn.width += 3;
                        } else {
                            btn.width += 1;
                        }
                    }
                    drawLightBevel(cr, style, state, (QtcRect*)area,
                                   btn.x, btn.y, btn.width, btn.height,
                                   &cols[bg], cols,
                                   rev ? ROUNDED_LEFT : ROUNDED_RIGHT,
                                   WIDGET_COMBO, BORDER_FLAT,
                                   (sunken ? DF_SUNKEN : 0) | DF_DO_BORDER |
                                   DF_HIDE_EFFECT, widget);
                } else if (opts.comboSplitter) {
                    if(sunken)
                        cx++, cy++, cheight--;

                    drawFadedLine(
                        cr, cx + (rev ? ind_width + style->xthickness :
                                  cwidth - ind_width - style->xthickness),
                        cy + style->ythickness - 1, 1, cheight - 3,
                        &btnColors[darkLine], (QtcRect*)area, nullptr,
                        true, true, false);

                    if (!sunken) {
                        drawFadedLine(
                            cr, cx + 1 + (rev ? ind_width + style->xthickness :
                                          cwidth - ind_width -
                                          style->xthickness),
                            cy + style->ythickness-1, 1, cheight - 3,
                            &btnColors[0], (QtcRect*)area, nullptr,
                            true, true, false);
                    }
                }
            } else if((button || togglebutton) && (combo || combo_entry)) {
                int vx = x + (width - (1 + (combo_entry ? 24 : 20)));
                /* int vwidth = width - (vx - x); */
                int darkLine = BORDER_VAL(GTK_STATE_INSENSITIVE != state);

                if (rev) {
                    vx = x + LARGE_ARR_WIDTH;
                    if (combo_entry) {
                        vx += 2;
                    }
                }

                if (opts.buttonEffect != EFFECT_NONE)
                    vx -= 2;

                if (!combo_entry) {
                    if (opts.comboBtn != SHADE_NONE) {
                        const GdkColor *cols =
                            (qtcPalette.combobtn &&
                             state != GTK_STATE_INSENSITIVE ?
                             qtcPalette.combobtn : btnColors);
                        int bg = SHADE_DARKEN==opts.comboBtn ||
                            (GTK_STATE_INSENSITIVE==state && SHADE_NONE!=opts.comboBtn) ? getFill(state, btnDown, true) : bgnd;

                        QtcRect btn = {
                            vx + (rev ? LARGE_ARR_WIDTH + 4 : 0),
                            y, 20 + 3, height
                        };
                        Cairo::Saver saver(cr);
                        if (!opts.comboSplitter) {
                            Cairo::clipRect(cr, &btn);
                        }
                        if (rev) {
                            btn.width += 3;
                        } else {
                            btn.x -= 3;
                            if (opts.buttonEffect != EFFECT_NONE) {
                                btn.width += 3;
                            }
                        }
                        drawLightBevel(cr, style, state, (QtcRect*)area, btn.x, btn.y,
                                       btn.width, btn.height, &cols[bg], cols,
                                       rev ? ROUNDED_LEFT : ROUNDED_RIGHT,
                                       WIDGET_COMBO, BORDER_FLAT,
                                       (sunken ? DF_SUNKEN : 0) | DF_DO_BORDER |
                                       DF_HIDE_EFFECT, widget);
                    } else if (opts.comboSplitter) {
                        drawFadedLine(cr, vx + (rev ? LARGE_ARR_WIDTH + 4 : 0),
                                      y + 4, 1, height - 8,
                                      &btnColors[darkLine], (QtcRect*)area,
                                      nullptr, true, true, false);

                        if(!sunken)
                            drawFadedLine(cr, vx + 1 +
                                          (rev ? LARGE_ARR_WIDTH + 4 : 0),
                                          y + 4, 1, height - 8, &btnColors[0],
                                          (QtcRect*)area, nullptr,
                                          true, true, false);
                    }
                }
            }
        }
    } else if (strcmp(detail, "buttondefault") == 0 ||
               strcmp(detail, "togglebuttondefault") == 0) {
    } else if (widget && (strcmp(detail, "trough") == 0 ||
                          qtcStrStartsWith(detail, "trough-"))) {
        bool list = isList(widget);
        bool pbar = list || GTK_IS_PROGRESS_BAR(widget);
        bool scale = !pbar && GTK_IS_SCALE(widget);
        /* int border = BORDER_VAL(GTK_STATE_INSENSITIVE != state || !scale); */
        bool horiz = (GTK_IS_RANGE(widget) ? Widget::isHorizontal(widget) :
                      width > height);

        if (scale) {
            drawSliderGroove(cr, style, state, widget, detail, (QtcRect*)area,
                             x, y, width, height, horiz);
        } else if (pbar) {
            drawProgressGroove(cr, style, state, window, widget, (QtcRect*)area,
                               x, y, width, height, list, horiz);
        } else {
            drawScrollbarGroove(cr, style, state, widget, (QtcRect*)area,
                                x, y, width, height, horiz);
        }
    } else if (DETAIL("entry-progress")) {
        int adjust = (opts.fillProgress ? 4 : 3) - (opts.etchEntry ? 1 : 0);
        drawProgress(cr, style, state, widget, (QtcRect*)area, x - adjust,
                     y - adjust, width + adjust, height + 2 * adjust,
                     rev, true);
    } else if (strcmp(detail,"dockitem") == 0 ||
               strcmp(detail,"dockitem_bin") == 0) {
        if (qtcIsCustomBgnd(opts) && widget) {
            drawWindowBgnd(cr, style, (QtcRect*)area, window, widget,
                           x, y, width, height);
        }
    } else if (widget && ((menubar || strcmp(detail, "toolbar") == 0 ||
                           strcmp(detail, "handlebox") == 0 ||
                           strcmp(detail,"handlebox_bin") == 0) ||
                          widgetIsType(widget, "PanelAppletFrame"))) {
        //if(GTK_SHADOW_NONE!=shadow)
        {
            const GdkColor *col = menubar && (GTK_STATE_INSENSITIVE!=state || SHADE_NONE!=opts.shadeMenubars)
                                ? &menuColors(activeWindow)[ORIGINAL_SHADE]
                                : &style->bg[state];
            EAppearance app=menubar ? opts.menubarAppearance : opts.toolbarAppearance;
            int         menuBarAdjust=0,
                        opacity=getOpacity(widget);
            double      alpha=opacity!=100 ? (opacity/100.00) : 1.0;
            bool drawGradient = shadow != GTK_SHADOW_NONE && !qtcIsFlat(app);
            bool fillBackground = menubar && opts.shadeMenubars != SHADE_NONE;

            if ((menubar && opts.windowDrag) ||
                opts.windowDrag > WM_DRAG_MENUBAR) {
                WMMove::setup(widget);
            }

            if (menubar && BLEND_TITLEBAR) {
                menuBarAdjust = qtcGetWindowBorderSize(false).titleHeight;
                if (widget && Menu::emitSize(widget, height) &&
                    (opts.menubarHiding ||
                     opts.windowBorder &
                     WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR)) {
                    Window::menuBarDBus(widget, height);
                }
            }

            if (widget && (opacity!=100 || qtcIsCustomBgnd(opts))) {
                drawWindowBgnd(cr, style, (QtcRect*)area, window, widget,
                               x, y, width, height);
            }
            if (drawGradient) {
                drawBevelGradient(cr, (QtcRect*)area, x, y - menuBarAdjust, width,
                                  height + menuBarAdjust, col,
                                  (menubar ? true : DETAIL("handlebox") ?
                                   width < height : width > height),
                                  false, MODIFY_AGUA(app), WIDGET_OTHER, alpha);
            } else if (fillBackground) {
                Cairo::rect(cr, (QtcRect*)area, x, y, width, height,
                            col, alpha);
            }
            if (shadow != GTK_SHADOW_NONE && opts.toolbarBorders != TB_NONE) {
                drawToolbarBorders(cr, state, x, y, width, height,
                                   menubar && activeWindow, detail);
            }
        }
    } else if (widget && pbar) {
        drawProgress(cr, style, state, widget, (QtcRect*)area,
                     x, y, width, height, rev, false);
    } else if(DETAIL("menuitem")) {
        drawMenuItem(cr, state, style, widget, (QtcRect*)area,
                     x, y, width, height);
    } else if(DETAIL("menu")) {
        drawMenu(cr, widget, (QtcRect*)area, x, y, width, height);
    } else if ((!strcmp(detail, "paned") || !strcmp(detail + 1, "paned"))) {
        gtkDrawHandle(style, window, state, shadow, area, widget, detail,
                      x, y, width, height,
                      *detail == 'h' ? GTK_ORIENTATION_VERTICAL :
                      GTK_ORIENTATION_HORIZONTAL);
    } else if (strcmp(detail + 1, "ruler") == 0) {
        drawBevelGradient(cr, (QtcRect*)area, x, y, width, height,
                          &qtcPalette.background[ORIGINAL_SHADE],
                          detail[0] == 'h', false, opts.lvAppearance,
                          WIDGET_LISTVIEW_HEADER);

//        if(qtcIsFlatBgnd(opts.bgndAppearance) || !widget || !drawWindowBgnd(cr, style, area, widget, x, y, width, height))
//        {
//             drawAreaColor(cr, area, &style->bg[state], x, y, width, height);
//             if(widget && IMG_NONE!=opts.bgndImage.type)
//                 drawWindowBgnd(cr, style, area, widget, x, y, width, height);
//        }
    } else if (DETAIL("hseparator")) {
        bool isMenuItem = widget && GTK_IS_MENU_ITEM(widget);
        const GdkColor *cols=qtcPalette.background;
        int offset=opts.menuStripe && (isMozilla() || isMenuItem) ? 20 : 0;

        if(offset && isFakeGtk())
            offset+=2;

        if (isMenuItem && (opts.lighterPopupMenuBgnd || opts.shadePopupMenu)) {
            cols = qtcPalette.menu;
        }

        drawFadedLine(cr, x + 1 + offset, y + height / 2, width - (1 + offset),
                      1, &cols[isMenuItem ? MENU_SEP_SHADE : QTC_STD_BORDER],
                      (QtcRect*)area, nullptr, true, true, true);
    } else if (DETAIL("vseparator")) {
        drawFadedLine(cr, x + width / 2, y, 1, height,
                      &qtcPalette.background[QTC_STD_BORDER], (QtcRect*)area,
                      nullptr, true, true, false);
    } else {
        EWidget wt = (!_detail && GTK_IS_TREE_VIEW(widget) ?
                      WIDGET_PBAR_TROUGH : WIDGET_FRAME);
        Cairo::Saver saver(cr);
        qtcClipPath(cr, x + 1, y + 1, width - 2, height - 2,
                    WIDGET_OTHER, RADIUS_INTERNAL, round);
        if (qtcIsFlatBgnd(opts.bgndAppearance) || !widget ||
            !drawWindowBgnd(cr, style, (QtcRect*)area, window, widget,
                            x + 1, y + 1, width - 2, height - 2)) {
            Cairo::rect(cr, (QtcRect*)area, x + 1, y + 1,
                        width - 2, height - 2, &style->bg[state]);
            if (widget && opts.bgndImage.type != IMG_NONE) {
                drawWindowBgnd(cr, style, (QtcRect*)area, window, widget, x, y,
                               width, height);
            }
        }
        saver.restore();

        if (wt == WIDGET_PBAR_TROUGH) {
            drawProgressGroove(cr, style, state, window, widget,
                               (QtcRect*)area, x, y, width, height, true, true);
        } else {
            drawBorder(cr, style, state, (QtcRect*)area, x, y, width, height,
                       nullptr, (menuScroll || opts.square & SQUARE_FRAME ?
                              ROUNDED_NONE : ROUNDED_ALL),
                       shadowToBorder(shadow), wt, QTC_STD_BORDER);
        }
    }
    cairo_destroy(cr);
}

static void
gtkDrawBox(GtkStyle *style, GdkWindow *window, GtkStateType state,
           GtkShadowType shadow, GdkRectangle *area, GtkWidget *widget,
           const char *detail, int x, int y, int width, int height)
{
    sanitizeSize(window, &width, &height);
    drawBox(style, window, state, shadow, area, widget, detail, x, y,
            width, height,
            state == GTK_STATE_ACTIVE || shadow == GTK_SHADOW_IN);
}

static void
gtkDrawShadow(GtkStyle *style, GdkWindow *window, GtkStateType state,
              GtkShadowType shadow, GdkRectangle *area, GtkWidget *widget,
              const char *_detail, int x, int y, int width, int height)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    sanitizeSize(window, &width, &height);
    cairo_t *cr = Cairo::gdkCreateClip(window, area);
    bool comboBoxList = isComboBoxList(widget);
    bool comboList = !comboBoxList && isComboList(widget);

    if (comboBoxList || comboList) {
        bool square = opts.square & SQUARE_POPUP_MENUS;

        if ((!square || opts.popupBorder) &&
            (!comboList || !DETAIL("viewport"))) {
            bool nonGtk = square || isFakeGtk();
            bool composActive = !nonGtk && compositingActive(widget);
            bool isAlphaWidget = (!nonGtk && composActive &&
                                  isRgbaWidget(widget));
            bool useAlpha = !nonGtk && qtSettings.useAlpha && isAlphaWidget;

            if (opts.popupBorder && square) {
                Cairo::rect(cr, (QtcRect*)area, x, y, width, height,
                            &style->base[state]);
                cairo_new_path(cr);
                cairo_rectangle(cr, x + 0.5, y + 0.5, width - 1, height - 1);
                Cairo::setColor(cr, &qtcPalette.background[QTC_STD_BORDER]);
                cairo_stroke(cr);
            } else /* if (!opts.popupBorder || */
                   /*     !(opts.square & SQUARE_POPUP_MENUS)) */ {
                if (useAlpha) {
                    cairo_rectangle(cr, x, y, width, height);
                    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
                    cairo_set_source_rgba(cr, 0, 0, 0, 1);
                    cairo_fill(cr);
                    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
                    clearRoundedMask(widget, false);
                } else {
                    createRoundedMask(widget, x, y, width, height,
                                      opts.round >= ROUND_FULL ? 5.0 : 2.5,
                                      false);
                }

                Cairo::clipWhole(cr, x, y, width, height,
                                 opts.round >= ROUND_FULL ? 5.0 : 2.5,
                                 ROUNDED_ALL);
                Cairo::rect(cr, (QtcRect*)area, x, y, width, height,
                            &style->base[state]);
                if(useAlpha)
                    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
                if (opts.popupBorder) {
                    Cairo::pathWhole(cr, x + 0.5, y + 0.5, width - 1,
                                     height - 1, (opts.round >= ROUND_FULL ?
                                                  5.0 : 2.5) - 1,
                                     ROUNDED_ALL);
                    Cairo::setColor(cr,
                                     &qtcPalette.background[QTC_STD_BORDER]);
                    cairo_stroke(cr);
                }
            }
        } else {
            Cairo::rect(cr, (QtcRect*)area, x, y, width, height,
                        &style->base[state]);
        }
    }
    else if(!opts.gtkComboMenus && !isMozilla() && isComboFrame(widget))
    {
        GdkColor newColors[TOTAL_SHADES + 1];
        GdkColor *btnColors;
        int bgnd = getFill(state, false); // TODO!!! btnDown???
        bool sunken = //btnDown || (GTK_IS_BUTTON(widget) && qtcButtonIsDepressed(widget)) ||
            state == GTK_STATE_ACTIVE || oneOf(bgnd, 2, 3);
        GtkWidget *parent = gtk_widget_get_parent(widget);
        GtkWidget *mapped = parent ? WidgetMap::getWidget(parent, 0) : nullptr;

        if(parent && ComboBox::isHovered(parent))
            state=GTK_STATE_PRELIGHT;

        if(QT_CUSTOM_COLOR_BUTTON(style))
        {
            shadeColors(&style->bg[state], newColors);
            btnColors=newColors;
        }
        else
            btnColors=qtcPalette.button[GTK_STATE_INSENSITIVE==state ? PAL_DISABLED : PAL_ACTIVE];

        drawLightBevel(cr, style, state, (QtcRect*)area, x, y, width + 4, height,
                       &btnColors[bgnd], btnColors, ROUNDED_LEFT,
                       WIDGET_TOOLBAR_BUTTON, BORDER_FLAT,
                       (sunken ? DF_SUNKEN : 0) | DF_DO_BORDER |
                       (ComboBox::hasFocus(widget, mapped) ? DF_HAS_FOCUS : 0),
                       widget);

        if(GTK_STATE_PRELIGHT!=state)
        {
            if(mapped && GTK_STATE_PRELIGHT==gtk_widget_get_state(mapped))
                state=GTK_STATE_PRELIGHT, gtk_widget_set_state(widget, GTK_STATE_PRELIGHT);
        }
        if(mapped && GTK_STATE_INSENSITIVE!=gtk_widget_get_state(widget))
            gtk_widget_queue_draw(mapped);

        WidgetMap::setup(parent, widget, 1);
        ComboBox::setup(widget, parent);
    }
    else if(DETAIL("entry") || DETAIL("text"))
    {
        GtkWidget *parent=widget ? gtk_widget_get_parent(widget) : nullptr;
        if (parent && isList(parent)) {
            // Dont draw shadow for entries in listviews...
            // Fixes RealPlayer's in-line editing of its favourites.
        } else {
            bool combo = isComboBoxEntry(widget);
            bool isSpin = !combo && isSpinButton(widget);
            bool rev = reverseLayout(widget) || (combo && parent &&
                                                 reverseLayout(parent));
            GtkWidget *btn = nullptr;
            /* GtkStateType savedState = state; */

#if GTK_CHECK_VERSION(2, 16, 0)
#if !GTK_CHECK_VERSION(2, 90, 0) /* Gtk3:TODO !!! */
            if (isSpin && widget &&
                width == Widget::getAllocation(widget).width) {
                int btnWidth, dummy;
                gdk_drawable_get_size(GTK_SPIN_BUTTON(widget)->panel, &btnWidth, &dummy);
                width-=btnWidth;
                if(rev)
                    x+=btnWidth;
            }
#endif
#endif
            if((opts.unifySpin && isSpin) || (combo && opts.unifyCombo))
                width+=(combo ? 4 : 2);

            // If we're a combo entry, and not prelight, check to see if the button is
            // prelighted, if so so are we!
            if(GTK_STATE_PRELIGHT!=state && combo && opts.unifyCombo && parent)
            {
                btn=getComboButton(parent);
                if (!btn && parent) {
                    btn = WidgetMap::getWidget(parent, 0);
                }
                if(btn && GTK_STATE_PRELIGHT==gtk_widget_get_state(btn))
                    state=GTK_STATE_PRELIGHT, gtk_widget_set_state(widget, GTK_STATE_PRELIGHT);
            }

#if GTK_CHECK_VERSION(2, 90, 0)
            if(!opts.unifySpin && isSpin)
                width-=16;
#endif
            drawEntryField(cr, style, state, window, widget, (QtcRect*)area,
                           x, y, width, height, combo || isSpin ? rev ?
                           ROUNDED_RIGHT : ROUNDED_LEFT : ROUNDED_ALL,
                           WIDGET_ENTRY);
            if(combo && opts.unifyCombo && parent)
            {
                if(btn && GTK_STATE_INSENSITIVE!=gtk_widget_get_state(widget))
                    gtk_widget_queue_draw(btn);

                if (QTC_COMBO_ENTRY(parent)) {
                    WidgetMap::setup(parent, widget, 1);
                }
            }
        }
    } else {
        bool frame = !_detail || strcmp(detail, "frame") == 0;
        bool scrolledWindow = DETAIL("scrolled_window");
        bool viewport = !scrolledWindow && strstr(detail, "viewport");
        bool drawSquare = ((frame && opts.square & SQUARE_FRAME) ||
                           (!viewport && !scrolledWindow &&
                            !_detail && !widget));
        bool statusBar = (isFakeGtk() ? frame : isStatusBarFrame(widget));
        bool checkScrollViewState = (opts.highlightScrollViews && widget &&
                                     GTK_IS_SCROLLED_WINDOW(widget));
        bool isHovered = false;
        bool hasFocus = false;
        GdkColor *cols = nullptr;

        if (qtSettings.debug == DEBUG_ALL) {
            printf(DEBUG_PREFIX "%s %d %d %d %d %d %d %s  ", __FUNCTION__,
                   state, shadow, x, y, width, height, _detail);
            debugDisplayWidget(widget, 10);
        }

        if(scrolledWindow && GTK_SHADOW_IN!=shadow && widget && GTK_IS_SCROLLED_WINDOW(widget) &&
           GTK_IS_TREE_VIEW(gtk_bin_get_child(GTK_BIN(widget))))
            gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget), GTK_SHADOW_IN), shadow=GTK_SHADOW_IN;
        else if(frame && !statusBar && GTK_IS_FRAME(widget))
        {
            /*
            check for scrolled windows embedded in frames, that contain a treeview.
            if found, change the shadowtypes for consistency with normal -sunken- scrolled windows.
            this should improve rendering of most mandriva drake tools
            */
            GtkWidget *child=gtk_bin_get_child(GTK_BIN(widget));
            if(GTK_IS_SCROLLED_WINDOW(child) && GTK_IS_TREE_VIEW(gtk_bin_get_child(GTK_BIN(child))))
            {
                gtk_frame_set_shadow_type(GTK_FRAME(widget), GTK_SHADOW_NONE);
                shadow=GTK_SHADOW_NONE;

                // also change scrolled window shadow if needed
                GtkScrolledWindow *sw=GTK_SCROLLED_WINDOW(child);
                if(GTK_SHADOW_IN!=gtk_scrolled_window_get_shadow_type(sw))
                    gtk_scrolled_window_set_shadow_type(sw, GTK_SHADOW_IN);
            }
        }

        if (checkScrollViewState) {
            ScrolledWindow::setup(widget);
            isHovered = ScrolledWindow::hovered(widget);
            hasFocus = !isHovered && ScrolledWindow::hasFocus(widget);
        }

        cols=isHovered ? qtcPalette.mouseover
                       : hasFocus
                            ? qtcPalette.focus
                            : qtcPalette.background;
        if(!statusBar && !drawSquare && (frame || scrolledWindow || viewport/* || drawSquare*/)) // && ROUNDED)
        {
            if (shadow != GTK_SHADOW_NONE &&
                (!frame || opts.drawStatusBarFrames || !isFakeGtk())) {
                GtkWidget *parent = (widget ? gtk_widget_get_parent(widget) :
                                     nullptr);
                bool doBorder = !viewport && !drawSquare;
                bool windowFrame = (parent && !isFixedWidget(widget) &&
                                    GTK_IS_FRAME(widget) &&
                                    GTK_IS_WINDOW(parent));

                if (windowFrame) {
                    QtcRect wAlloc = Widget::getAllocation(widget);
                    QtcRect pAlloc = Widget::getAllocation(parent);
                    windowFrame = Rect::equal(wAlloc, pAlloc);
                }

//                 if(!drawSquare && widget && gtk_widget_get_parent(widget) && !isFixedWidget(widget) &&
//                    GTK_IS_FRAME(widget) && GTK_IS_WINDOW(gtk_widget_get_parent(widget)))
//                     drawSquare=true;

                if(scrolledWindow)
                {
                    /* See code in qt_settings.c as to isMozill part */
                    if((opts.square&SQUARE_SCROLLVIEW && !opts.highlightScrollViews) || isMozillaWidget(widget))
                    {
                        /* Flat style...
                        drawBorder(cr, style, state, area, x, y, width, height,
                                   nullptr, ROUNDED_NONE, BORDER_FLAT, WIDGET_SCROLLVIEW, 0);
                        */
                        /* 3d... */
                        Cairo::setColor(cr, &cols[QTC_STD_BORDER]);
                        Cairo::pathTopLeft(cr, x + 0.5, y + 0.5, width - 1,
                                           height - 1, 0.0, ROUNDED_NONE);
                        cairo_stroke(cr);
                        if(!opts.gtkScrollViews)
                            Cairo::setColor(cr, &cols[QTC_STD_BORDER],
                                             LOWER_BORDER_ALPHA);
                            /* Cairo::setColor(cr, &qtcPalette.background[ */
                            /*                      STD_BORDER_BR]); */
                        Cairo::pathBottomRight(cr, x + 0.5, y + 0.5, width - 1,
                                               height - 1, 0, ROUNDED_NONE);
                        cairo_stroke(cr);
                        doBorder=false;
                    } else if (opts.etchEntry) {
                        drawEtch(cr, (QtcRect*)area, widget, x, y, width,
                                 height, false, ROUNDED_ALL, WIDGET_SCROLLVIEW);
                        x++;
                        y++;
                        width -= 2;
                        height -= 2;
                    }
                }
                if (viewport || windowFrame/* || drawSquare*/) {
                    cairo_new_path(cr);
                    cairo_rectangle(cr, x + 0.5, y + 0.5,
                                    width - 1, height - 1);
                    if (windowFrame) {
                        Cairo::setColor(cr, &cols[QTC_STD_BORDER]);
                    } else {
                        Cairo::setColor(cr, &cols[ORIGINAL_SHADE]);
                    }
                    cairo_stroke(cr);
                }
                else if(doBorder)
                    drawBorder(cr, style, state, (QtcRect*)area, x, y, width,
                               height, cols, ROUNDED_ALL,
                               scrolledWindow ? BORDER_SUNKEN : BORDER_FLAT,
                               scrolledWindow ? WIDGET_SCROLLVIEW :
                               WIDGET_FRAME, DF_BLEND);
            }
        } else if (!statusBar || opts.drawStatusBarFrames) {
            int c1 = 0;
            int c2 = 0;

            switch(shadow) {
            case GTK_SHADOW_NONE:
                if (statusBar) {
                    shadow = GTK_SHADOW_IN;
                } else {
                    break;
                }
            case GTK_SHADOW_IN:
            case GTK_SHADOW_ETCHED_IN:
                c1 = 0;
                c2 = QTC_STD_BORDER;
                break;
            case GTK_SHADOW_OUT:
            case GTK_SHADOW_ETCHED_OUT:
                c1 = QTC_STD_BORDER;
                c2 = 0;
                break;
            }

            switch(shadow) {
            case GTK_SHADOW_NONE:
                if (!frame) {
                    cairo_new_path(cr);
                    cairo_rectangle(cr, x + 0.5, y + 0.5,
                                    width - 1, height - 1);
                    Cairo::setColor(cr, &cols[QTC_STD_BORDER]);
                    cairo_stroke(cr);
                }
                break;
            case GTK_SHADOW_IN:
            case GTK_SHADOW_OUT:
                // if(drawSquare || frame || !ROUNDED)
            {
                double c2Alpha =
                    shadow == GTK_SHADOW_IN ? 1.0 : LOWER_BORDER_ALPHA;
                double c1Alpha =
                    shadow == GTK_SHADOW_OUT ? 1.0 : LOWER_BORDER_ALPHA;
                Cairo::hLine(cr, x, y, width,
                             &cols[QTC_STD_BORDER], c2Alpha);
                Cairo::vLine(cr, x, y, height,
                             &cols[QTC_STD_BORDER], c2Alpha);
                if (opts.appearance != APPEARANCE_FLAT) {
                    Cairo::hLine(cr, x, y + height - 1, width,
                                 &cols[QTC_STD_BORDER], c1Alpha);
                    Cairo::vLine(cr, x + width - 1, y, height,
                                 &cols[QTC_STD_BORDER], c1Alpha);
                }
                break;
            }
            case GTK_SHADOW_ETCHED_IN:
                cairo_new_path(cr);
                cairo_rectangle(cr, x + 1.5, y + 1.5, width - 2, height - 2);
                Cairo::setColor(cr, &cols[c1]);
                cairo_stroke(cr);
                cairo_new_path(cr);
                cairo_rectangle(cr, x + 0.5, y + 0.5, width - 2, height - 2);
                Cairo::setColor(cr, &cols[c2]);
                cairo_stroke(cr);
                break;
            case GTK_SHADOW_ETCHED_OUT:
                cairo_new_path(cr);
                cairo_rectangle(cr, x + 1.5, y + 1.5, width - 2, height - 2);
                Cairo::setColor(cr, &cols[c2]);
                cairo_stroke(cr);
                cairo_new_path(cr);
                cairo_rectangle(cr, x + 0.5, y + 0.5, width - 2, height - 2);
                Cairo::setColor(cr, &cols[c1]);
                cairo_stroke(cr);
                break;
            }
        }
    }
    cairo_destroy(cr);
}

// static gboolean isHoveredCell(GtkWidget *widget, int x, int y, int width, int height)
// {
//     gboolean hovered=false;
//
//     if(widget && GTK_IS_TREE_VIEW(widget))
//     {
//         GtkTreePath       *path=nullptr;
//         GtkTreeViewColumn *column=nullptr;
//
//         qtcTreeViewSetup(widget);
//         qtcTreeViewGetCell(GTK_TREE_VIEW(widget), &path, &column, x, y, width, height);
//         hovered=path && qtcTreeViewIsCellHovered(widget, path, column);
//     }
//
//     return hovered;
// }

static void
gtkDrawCheck(GtkStyle *style, GdkWindow *window, GtkStateType state,
             GtkShadowType shadow, GdkRectangle *_area, GtkWidget *widget,
             const char *_detail, int x, int y, int width, int height)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    QtcRect *area = (QtcRect*)_area;
    cairo_t *cr = Cairo::gdkCreateClip(window, area);
    drawCheckBox(cr, state, shadow, style, widget, detail, area,
                 x, y, width, height);
    cairo_destroy(cr);
}

static void
gtkDrawOption(GtkStyle *style, GdkWindow *window, GtkStateType state,
              GtkShadowType shadow, GdkRectangle *_area, GtkWidget *widget,
              const char *_detail, int x, int y, int width, int height)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    QtcRect *area = (QtcRect*)_area;
    cairo_t *cr = Cairo::gdkCreateClip(window, area);
    drawRadioButton(cr, state, shadow, style, widget, detail, area,
                    x, y, width, height);
    cairo_destroy(cr);
}

#define NUM_GCS 5

static void
drawLayout(cairo_t *cr, GtkStyle *style, GtkStateType state,
           bool use_text, const QtcRect *area, int x, int y,
           PangoLayout *layout)
{
    Cairo::layout(cr, area, x, y, layout,
                  use_text || state == GTK_STATE_INSENSITIVE ?
                  &style->text[state] : &style->fg[state]);
}

static void
gtkDrawLayout(GtkStyle *style, GdkWindow *window, GtkStateType state,
              gboolean use_text, GdkRectangle *_area, GtkWidget *widget,
              const char *_detail, int x, int y, PangoLayout *layout)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    const QtcRect *area = (QtcRect*)_area;
    cairo_t *cr = gdk_cairo_create(window);
    if (GTK_IS_PROGRESS(widget) || DETAIL("progressbar")) {
        drawLayout(cr, style, state, use_text, area, x, y, layout);
    } else {
        Style *qtc_style = (Style*)style;
        bool isMenuItem = IS_MENU_ITEM(widget);
        GtkMenuBar *mb = isMenuItem ? isMenubar(widget, 0) : nullptr;
        bool activeMb = mb ? GTK_MENU_SHELL(mb)->active : false;
        bool selectedText = ((opts.useHighlightForMenu ||
                              opts.customMenuTextColor) && isMenuItem &&
                             (opts.colorMenubarMouseOver ?
                              state == GTK_STATE_PRELIGHT :
                              ((!mb || activeMb) &&
                               state == GTK_STATE_PRELIGHT)));
        bool def_but = false;
        bool but = isOnButton(widget, 0, &def_but);
        bool swapColors = false;
        QtcRect area2;
        GtkWidget *parent = widget ? gtk_widget_get_parent(widget) : nullptr;

        if (!opts.colorMenubarMouseOver && mb && !activeMb &&
            state == GTK_STATE_PRELIGHT)
            state = GTK_STATE_NORMAL;

        GdkColor prevColors[NUM_GCS];

        if (qtSettings.debug == DEBUG_ALL) {
            printf(DEBUG_PREFIX "%s %s %d %d %d %d %d %s  ", __FUNCTION__,
                   pango_layout_get_text(layout), x, y, state, use_text,
                   IS_MENU_ITEM(widget), _detail);
            debugDisplayWidget(widget, 10);
        }

        if (DETAIL("cellrenderertext") && widget &&
            gtk_widget_get_state(widget) == GTK_STATE_INSENSITIVE)
             state = GTK_STATE_INSENSITIVE;

#ifndef READ_INACTIVE_PAL
        /* If we reead the inactive palette, then there is no need for the
           following... The following fixes the text in list views...
           if not used, when an item is selected it gets the selected text
           color - but when the window changes focus it gets the normal
           text color! */
         if (DETAIL("cellrenderertext") && state == GTK_STATE_ACTIVE)
             state = GTK_STATE_SELECTED;
#endif

        if (!isMenuItem && state == GTK_STATE_PRELIGHT)
            state = GTK_STATE_NORMAL;

        if (!use_text && parent && GTK_IS_LABEL(widget) &&
            (isOnOptionMenu(parent, 0) ||
             (GTK_IS_BUTTON(parent) && isOnMenuItem(parent, 0)))) {
            use_text = true;
        }

        /*
           This check of 'requisition' size (and not 'allocation') seems to
           match better with Qt4's text positioning. For example, 10pt verdana
           - no shift is required 9pt DejaVu Sans requires the shift
        */
        if (but && widget) {
            GtkRequisition req = Widget::getRequisition(widget);
            if (req.height < Widget::getAllocation(widget).height &&
                req.height % 2) {
                y++;
            }
        }

        but = but || isOnComboBox(widget, 0);

        if (isOnListViewHeader(widget, 0))
            y--;

        if (but && ((qtSettings.qt4 && state == GTK_STATE_INSENSITIVE) ||
                    (!qtSettings.qt4 && state != GTK_STATE_INSENSITIVE))) {
            use_text = true;
            swapColors = true;
            for (int i = 0;i < NUM_GCS;++i) {
                prevColors[i] = style->text[i];
                style->text[i] =
                    *qtc_style->button_text[state == GTK_STATE_INSENSITIVE ?
                                            PAL_DISABLED : PAL_ACTIVE];
            }
            if (state == GTK_STATE_INSENSITIVE) {
                state = GTK_STATE_NORMAL;
            }
        } else if (isMenuItem) {
            bool activeWindow =
                (mb && opts.shadeMenubarOnlyWhenActive && widget ?
                 Window::isActive(gtk_widget_get_toplevel(widget)) : true);

            if ((opts.shadePopupMenu && state == GTK_STATE_PRELIGHT) ||
                (mb && (activeWindow ||
                        opts.shadeMenubars == SHADE_WINDOW_BORDER))) {
                if (opts.shadeMenubars == SHADE_WINDOW_BORDER) {
                    for (int i = 0;i < NUM_GCS;i++) {
                        prevColors[i] = style->text[i];
                    }
                    swapColors = true;
                    style->text[GTK_STATE_NORMAL] =
                        *qtc_style->menutext[activeWindow ? 1 : 0];
                    use_text = true;
                } else if (opts.customMenuTextColor &&
                           qtc_style->menutext[0]) {
                    for (int i = 0;i < NUM_GCS;i++) {
                        prevColors[i] = style->text[i];
                    }
                    swapColors = true;
                    style->text[GTK_STATE_NORMAL] = *qtc_style->menutext[0];
                    style->text[GTK_STATE_ACTIVE] = *qtc_style->menutext[1];
                    style->text[GTK_STATE_PRELIGHT] = *qtc_style->menutext[0];
                    style->text[GTK_STATE_SELECTED] = *qtc_style->menutext[1];
                    style->text[GTK_STATE_INSENSITIVE] =
                        *qtc_style->menutext[0];
                    use_text = true;
                } else if (oneOf(opts.shadeMenubars, SHADE_BLEND_SELECTED,
                                 SHADE_SELECTED) ||
                           (opts.shadeMenubars == SHADE_CUSTOM &&
                            TOO_DARK(qtcPalette.menubar[ORIGINAL_SHADE]))) {
                    selectedText = true;
                }
            }
        }

        if (parent && GTK_IS_LABEL(widget) && GTK_IS_FRAME(parent) &&
            !isOnStatusBar(widget, 0)) {
            int diff = (Widget::getAllocation(widget).x -
                        Widget::getAllocation(parent).x);

            if (qtcNoFrame(opts.groupBox)) {
                x -= qtcBound(0, diff, 8);
            } else if (opts.gbLabel&GB_LBL_OUTSIDE) {
                x -= qtcBound(0, diff, 4);
            } else if(opts.gbLabel&GB_LBL_INSIDE) {
                x -= qtcBound(0, diff, 2);
            } else {
                x += 5;
            }
#if GTK_CHECK_VERSION(2, 90, 0)
            cairo_reset_clip(cr);
#else
            if (area) {
                area2 = *area;
                if (qtcNoFrame(opts.groupBox)) {
                    area2.x -= qtcBound(0, diff, 8);
                } else if (opts.gbLabel & GB_LBL_OUTSIDE) {
                    area2.x -= qtcBound(0, diff, 4);
                } else if(opts.gbLabel&GB_LBL_INSIDE) {
                    area2.x -= qtcBound(0, diff, 2);
                } else {
                    area2.x += 5;
                }
                area = &area2;
            }
#endif
        }

        if (!opts.useHighlightForMenu && (isMenuItem || mb) &&
            state != GTK_STATE_INSENSITIVE)
            state = GTK_STATE_NORMAL;

        drawLayout(cr, style, selectedText ? GTK_STATE_SELECTED : state,
                   use_text || selectedText, area, x, y, layout);

        if (opts.embolden && def_but) {
            drawLayout(cr, style, selectedText ? GTK_STATE_SELECTED : state,
                       use_text || selectedText, area, x + 1, y, layout);
        }

        if (swapColors) {
            for (int i = 0;i < 5;++i) {
                style->text[i] = prevColors[i];
            }
        }
    }
    cairo_destroy(cr);
}

static GdkPixbuf*
gtkRenderIcon(GtkStyle *style, const GtkIconSource *source,
              GtkTextDirection, GtkStateType state, GtkIconSize size,
              GtkWidget *widget, const char*)
{
    return renderIcon(style, source, state, size, widget);
}

static void
gtkDrawTab(GtkStyle*, GdkWindow *window, GtkStateType state,
           GtkShadowType shadow, GdkRectangle *_area, GtkWidget *widget,
           const char *_detail, int x, int y, int width, int height)
{
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %s  ", __FUNCTION__, state, shadow,
               _detail);
        debugDisplayWidget(widget, 10);
    }
    const QtcRect *area = (QtcRect*)_area;
    cairo_t *cr = gdk_cairo_create(window);
    const GdkColor *arrowColor =
        MO_ARROW(false, &qtSettings.colors[state == GTK_STATE_INSENSITIVE ?
                                           PAL_DISABLED :
                                           PAL_ACTIVE][COLOR_BUTTON_TEXT]);
    if (isActiveOptionMenu(widget)) {
        x++;
        y++;
    }
    x = (reverseLayout(widget) ||
         ((widget = gtk_widget_get_parent(widget)) && reverseLayout(widget)) ?
         x + 1 : x + width / 2);
    if (opts.doubleGtkComboArrow) {
        int pad = opts.vArrows ? 0 : 1;
        Cairo::arrow(cr, arrowColor, area,  GTK_ARROW_UP,
                     x, y + height / 2 - (LARGE_ARR_HEIGHT - pad),
                     false, true, opts.vArrows);
        Cairo::arrow(cr, arrowColor, area,  GTK_ARROW_DOWN,
                     x, y + height / 2 + (LARGE_ARR_HEIGHT - pad),
                     false, true, opts.vArrows);
    } else {
        Cairo::arrow(cr, arrowColor, area,  GTK_ARROW_DOWN,
                     x, y + height / 2, false, true, opts.vArrows);
    }
    cairo_destroy(cr);
}

static void
gtkDrawBoxGap(GtkStyle *style, GdkWindow *window, GtkStateType state,
              GtkShadowType, GdkRectangle *area, GtkWidget *widget,
              const char *_detail, int x, int y, int width, int height,
              GtkPositionType gapSide, int gapX, int gapWidth)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    cairo_t *cr = Cairo::gdkCreateClip(window, area);

    if ((opts.thin & THIN_FRAMES) && gapX == 0) {
        gapX--;
        gapWidth += 2;
    }

    sanitizeSize(window, &width, &height);
    drawBoxGap(cr, style, GTK_SHADOW_OUT, state, widget, (QtcRect*)area, x, y,
               width, height, gapSide, gapX, gapWidth,
               opts.borderTab ? BORDER_LIGHT : BORDER_RAISED, true);

    if (opts.windowDrag > WM_DRAG_MENU_AND_TOOLBAR && DETAIL("notebook")) {
        WMMove::setup(widget);
    }

    if (!isMozilla())
        drawBoxGapFixes(cr, widget, x, y, width, height,
                        gapSide, gapX, gapWidth);
    cairo_destroy(cr);
}

static void
gtkDrawExtension(GtkStyle *style, GdkWindow *window, GtkStateType state,
                 GtkShadowType shadow, GdkRectangle *_area, GtkWidget *widget,
                 const char *_detail, int x, int y, int width, int height,
                 GtkPositionType gapSide)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %d %d %s  ", __FUNCTION__, state,
               shadow, gapSide, x, y, width, height, _detail);
        debugDisplayWidget(widget, 10);
    }
    sanitizeSize(window, &width, &height);

    if (DETAIL("tab")) {
        QtcRect *area = (QtcRect*)_area;
        cairo_t *cr = Cairo::gdkCreateClip(window, area);
        drawTab(cr, state, style, widget, area, x, y, width, height, gapSide);
        cairo_destroy(cr);
    } else {
        parent_class->draw_extension(style, window, state, shadow, _area,
                                     widget, _detail, x, y, width, height,
                                     gapSide);
    }
}

static void
gtkDrawSlider(GtkStyle *style, GdkWindow *window, GtkStateType state,
              GtkShadowType shadow, GdkRectangle *area, GtkWidget *widget,
              const char *_detail, int x, int y, int width, int height,
              GtkOrientation orientation)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    bool scrollbar = DETAIL("slider");
    bool scale = oneOf(detail, "hscale", "vscale");

    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %d %s  ", __FUNCTION__, state,
               shadow, x, y, width, height, _detail);
        debugDisplayWidget(widget, 10);
    }

    cairo_t *cr = Cairo::gdkCreateClip(window, area);
    sanitizeSize(window, &width, &height);

    if (scrollbar || SLIDER_TRIANGULAR!=opts.sliderStyle) {
        GdkColor newColors[TOTAL_SHADES + 1];
        GdkColor *btnColors = qtcPalette.background;
        int min = MIN_SLIDER_SIZE(opts.sliderThumbs);

#ifdef INCREASE_SB_SLIDER
        if (scrollbar)
            lastSlider.widget = nullptr;
#endif
        /* Fix Java swing sliders looking pressed */
        if(!scrollbar && GTK_STATE_ACTIVE==state)
            state=GTK_STATE_PRELIGHT;

        // FIXME, need to update useButtonColor if the logic below changes
        if (useButtonColor(detail)) {
            if(scrollbar|scale && GTK_STATE_INSENSITIVE==state)
                btnColors=qtcPalette.background;
            else if(QT_CUSTOM_COLOR_BUTTON(style))
            {
                shadeColors(&style->bg[state], newColors);
                btnColors=newColors;
            }
            else
                SET_BTN_COLS(scrollbar, scale, false, state)
        }

#ifdef INCREASE_SB_SLIDER
        if(scrollbar && !opts.flatSbarButtons && SHADE_NONE!=opts.shadeSliders && SCROLLBAR_NONE!=opts.scrollbarType && !isMozilla())
        {
            lastSlider.style=style;
#if GTK_CHECK_VERSION(2, 90, 0)
            lastSlider.cr=cr;
#else
            lastSlider.window=window;
#endif
            lastSlider.state=state;
            lastSlider.shadow=shadow;
            lastSlider.widget=widget;
            lastSlider.detail=detail;
            lastSlider.x=x;
            lastSlider.y=y;
            lastSlider.width=width;
            lastSlider.height=height;
            lastSlider.orientation=orientation;
        }
#endif

#if GTK_CHECK_VERSION(2, 90, 0)
        if(scrollbar && GTK_STATE_ACTIVE==state)
            state=GTK_STATE_PRELIGHT;
#endif

        drawBox(style, window, state, shadow, area, widget,
                !scrollbar ? "qtc-slider" : "slider",
                x, y, width, height, false);

       /* Orientation is always vertical with Mozilla, why? Anyway this hack
          should be OK - as we only draw dashes when slider is larger than
          'min' pixels... */
        orientation = (width < height ? GTK_ORIENTATION_VERTICAL :
                       GTK_ORIENTATION_HORIZONTAL);
        if (opts.sliderThumbs != LINE_NONE &&
            (scrollbar || opts.sliderStyle != SLIDER_CIRCULAR) &&
            (scale || ((orientation == GTK_ORIENTATION_HORIZONTAL &&
                        width >= min) || height >= min))) {
            GdkColor *markers=/*opts.coloredMouseOver && GTK_STATE_PRELIGHT==state
                                ? qtcPalette.mouseover
                                : */btnColors;
            bool horiz = orientation == GTK_ORIENTATION_HORIZONTAL;

            if (opts.sliderThumbs == LINE_SUNKEN) {
                if (horiz) {
                    y--;
                    height++;
                } else {
                    x--;
                    width++;
                }
            } else if (horiz) {
                x++;
            } else {
                y++;
            }

            switch (opts.sliderThumbs) {
            case LINE_1DOT:
                Cairo::dot(cr, x, y, width, height,
                           &markers[QTC_STD_BORDER]);
                break;
            case LINE_FLAT:
                drawLines(cr, x, y, width, height, !horiz, 3, 5, markers,
                          (QtcRect*)area, 5, opts.sliderThumbs);
                break;
            case LINE_SUNKEN:
                drawLines(cr, x, y, width, height, !horiz, 4, 3, markers,
                          (QtcRect*)area, 3, opts.sliderThumbs);
                break;
            default:
            case LINE_DOTS:
                Cairo::dots(cr, x, y, width, height, !horiz,
                            scale ? 3 : 5, scale ? 4 : 2, (QtcRect*)area,
                            0, &markers[5], markers);
            }
        }
    } else {
        drawTriangularSlider(cr, style, state, detail, x, y, width, height);
    }
    cairo_destroy(cr);
}

static void
gtkDrawShadowGap(GtkStyle *style, GdkWindow *window, GtkStateType state,
                 GtkShadowType shadow, GdkRectangle *_area, GtkWidget *widget,
                 const char*, int x, int y, int width, int height,
                 GtkPositionType gapSide, int gapX, int gapWidth)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    QtcRect *area = (QtcRect*)_area;
    cairo_t *cr = Cairo::gdkCreateClip(window, area);
    sanitizeSize(window, &width, &height);
    drawShadowGap(cr, style, shadow, state, widget, area, x, y,
                  width, height, gapSide, gapX, gapWidth);
    cairo_destroy(cr);
}

static void
gtkDrawHLine(GtkStyle *style, GdkWindow *window, GtkStateType state,
             GdkRectangle *area, GtkWidget *widget, const char *_detail,
             int x1, int x2, int y)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";
    bool tbar = strcmp(detail, "toolbar");
    int light = 0;
    int dark = tbar ? (opts.toolbarSeparators == LINE_FLAT ? 4 : 3) : 5;

    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %s  ", __FUNCTION__, state, x1, x2,
               y, _detail);
        debugDisplayWidget(widget, 10);
    }

    cairo_t *cr = Cairo::gdkCreateClip(window, area);

    if (tbar) {
        switch (opts.toolbarSeparators) {
            default:
            case LINE_DOTS:
                Cairo::dots(cr, x1, y, x2 - x1, 2, false,
                            (x2 - x1) / 3.0 + 0.5, 0, (QtcRect*)area, 0,
                            &qtcPalette.background[5], qtcPalette.background);
                break;
            case LINE_NONE:
                break;
            case LINE_FLAT:
            case LINE_SUNKEN:
            {
                drawFadedLine(cr, x1 < x2 ? x1 : x2, y, abs(x2 - x1), 1,
                              &qtcPalette.background[dark], (QtcRect*)area,
                              nullptr, true, true, true);
                /* Cairo::hLine(cr, x1 < x2 ? x1 : x2, y, abs(x2 - x1), */
                /*              &qtcPalette.background[dark]); */
                if(LINE_SUNKEN==opts.toolbarSeparators)
                {
                    cairo_new_path(cr);
                    /* Cairo::hLine(cr, x1 < x2 ? x1 : x2, y + 1, abs(x2 - x1), */
                    /*              &qtcPalette.background[light]); */
                    drawFadedLine(cr, x1 < x2 ? x1 : x2, y + 1, abs(x2 - x1), 1,
                                  &qtcPalette.background[light], (QtcRect*)area,
                                  nullptr, true, true, true);
                }
            }
        }
    } else if (DETAIL("label")) {
        if (state == GTK_STATE_INSENSITIVE) {
            /* Cairo::hLine(cr, (x1 < x2 ? x1 : x2) + 1, y + 1, abs(x2 - x1), */
            /*              &qtcPalette.background[light]); */
            drawFadedLine(cr, x1 < x2 ? x1 : x2, y + 1, abs(x2 - x1), 1,
                          &qtcPalette.background[light], (QtcRect*)area, nullptr,
                          true, true, true);
        }
        /* Cairo::hLine(cr, x1 < x2 ? x1 : x2, y, abs(x2 - x1), */
        /*              &style->text[state]); */
        drawFadedLine(cr, x1 < x2 ? x1 : x2, y, abs(x2 - x1), 1,
                      &qtcPalette.background[dark], (QtcRect*)area, nullptr,
                      true, true, true);
    }
    else if(DETAIL("menuitem") || (widget && DETAIL("hseparator") && IS_MENU_ITEM(widget)))
    {
        int       offset=opts.menuStripe && (isMozilla() || (widget && GTK_IS_MENU_ITEM(widget))) ? 20 : 0;
        GdkColor *cols=qtcPalette.background;

        if(offset && isFakeGtk())
            offset+=2;

        if (opts.lighterPopupMenuBgnd || opts.shadePopupMenu) {
            cols = qtcPalette.menu;
        }
        if (offset && isFakeGtk()) {
            offset += 2;
        }

        /* Cairo::hLine(cr, x1 < x2 ? x1 : x2, y, abs(x2 - x1), */
        /*              &qtcPalette.background[MENU_SEP_SHADE]); */
        drawFadedLine(cr, offset + (x1 < x2 ? x1 : x2), y + 1,
                      abs(x2 - x1) - offset, 1, &cols[MENU_SEP_SHADE],
                      (QtcRect*)area, nullptr, true, true, true);
    } else {
        /* Cairo::hLine(cr, x1 < x2 ? x1 : x2, y, abs(x2 - x1), */
        /*              qtcPalette.background[dark]); */
        drawFadedLine(cr, x1 < x2 ? x1 : x2, y, abs(x2 - x1), 1,
                      &qtcPalette.background[dark], (QtcRect*)area, nullptr,
                      true, true, true);
    }

    cairo_destroy(cr);
}

static void
gtkDrawVLine(GtkStyle *style, GdkWindow *window, GtkStateType state,
             GdkRectangle *area, GtkWidget *widget, const char *_detail,
             int y1, int y2, int x)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    const char *detail = _detail ? _detail : "";

    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %s  ", __FUNCTION__, state, x, y1,
               y2, _detail);
        debugDisplayWidget(widget, 10);
    }

    cairo_t *cr = Cairo::gdkCreateClip(window, area);

    if (!(DETAIL("vseparator") && isOnComboBox(widget, 0))) {
         /* CPD: Combo handled in drawBox */
        bool tbar = DETAIL("toolbar");
        int dark = tbar ? 3 : 5;
        int light = 0;

        if (tbar) {
            switch (opts.toolbarSeparators) {
            default:
            case LINE_DOTS:
                Cairo::dots(cr, x, y1, 2, y2 - y1, true,
                            (y2 - y1) / 3.0 + 0.5, 0, (QtcRect*)area, 0,
                            &qtcPalette.background[5],
                            qtcPalette.background);
                break;
            case LINE_NONE:
                break;
            case LINE_FLAT:
            case LINE_SUNKEN:
                /* Cairo::vLine(cr, x, y1 < y2 ? y1 : y2, abs(y2 - y1), */
                /*              &qtcPalette.background[dark]); */
                drawFadedLine(cr, x, y1 < y2 ? y1 : y2, 1, abs(y2 - y1),
                              &qtcPalette.background[dark],
                              (QtcRect*)area, nullptr, true, true, false);
                if (opts.toolbarSeparators == LINE_SUNKEN)
                    /* Cairo::vLine(cr, x + 1, y1 < y2 ? y1 : y2, */
                    /*              abs(y2 - y1), */
                    /*              &qtcPalette.background[light]); */
                    drawFadedLine(cr, x + 1, y1 < y2 ? y1 : y2, 1,
                                  abs(y2 - y1), &qtcPalette.background[light],
                                  (QtcRect*)area, nullptr, true, true, false);
            }
        } else {
            /* Cairo::vLine(cr, x, y1 < y2 ? y1 : y2, abs(y2 - y1), */
            /*              &qtcPalette.background[dark]); */
            drawFadedLine(cr, x, y1 < y2 ? y1 : y2, 1, abs(y2 - y1),
                          &qtcPalette.background[dark], (QtcRect*)area,
                          nullptr, true, true, false);
        }
    }
    cairo_destroy(cr);
}

static void
gtkDrawFocus(GtkStyle *style, GdkWindow *window, GtkStateType state,
             GdkRectangle *area, GtkWidget *widget, const char *_detail,
             int x, int y, int width, int height)
{
    if (opts.focus == FOCUS_NONE) {
        return;
    }
    if (GTK_IS_EDITABLE(widget))
        return;
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));

    sanitizeSize(window, &width, &height);

    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %s ", __FUNCTION__, state, x, y,
               width, height, _detail);
        debugDisplayWidget(widget, 10);
    }
    GtkWidget *parent = widget ? gtk_widget_get_parent(widget) : nullptr;
    bool doEtch = opts.buttonEffect != EFFECT_NONE;
    bool btn = false;
    bool comboButton = false;
    bool rev = parent && reverseLayout(parent);
    bool view = isList(widget);
    bool listViewHeader = isListViewHeader(widget);
    bool toolbarBtn = (!listViewHeader && !view &&
                       isButtonOnToolbar(widget, nullptr));

    if (opts.comboSplitter &&
        noneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) && isComboBox(widget)) {
        /* x++; */
        /* y++; */
        /* height -= 2; */
        width += 2; /* Remove if re-add the above */

        if (widget && rev) {
            x += 20;
        }
        width -= 22;

        if (isGimpCombo(widget)) {
            x += 2;
            y += 2;
            width -= 4;
            height -= 4;
        }
        btn = true;
    }
#if !GTK_CHECK_VERSION(2, 90, 0)
    else if (GTK_IS_OPTION_MENU(widget)) {
        if ((!opts.comboSplitter ||
             oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED)) && widget) {
            QtcRect alloc = Widget::getAllocation(widget);
            if (alloc.width > width) {
                width = alloc.width - (doEtch ? 8 : 4);
            }
        }
        x++;
        y++;
        width -= 2;
        height -= 2;
        btn = true;
    }
#endif
    if (isComboBoxEntryButton(widget)) {
#if GTK_CHECK_VERSION(2, 90, 0)
        if (doEtch) {
            x -= 2;
            y++;
            width += 3;
            height -= 2;
        } else {
            x -= 2;
            width += 4;
        }
#else
        if (doEtch) {
            x++;
            y += 2;
            width -= 3;
            height -= 4;
        } else {
            x++;
            y++;
            width -= 2;
            height -= 2;
        }
#endif
        comboButton = true;
        btn = true;
    } else if (isGimpCombo(widget)) {
        if (opts.focus == FOCUS_GLOW) {
            return;
        }
        x -= 2;
        width += 4;
        if (!doEtch) {
            x--;
            y--;
            width += 2;
            height += 2;
        }
    } else if (GTK_IS_BUTTON(widget)) {
        if (GTK_IS_RADIO_BUTTON(widget) || GTK_IS_CHECK_BUTTON(widget)) {
            // Gimps buttons in its toolbox are
            const char *text = nullptr;
            toolbarBtn = (qtSettings.app == GTK_APP_GIMP &&
                          ((text =
                            gtk_button_get_label(GTK_BUTTON(widget))) == nullptr ||
                           text[0] == '\0'));

            if (!toolbarBtn && opts.focus == FOCUS_GLOW && !isMozilla()) {
                return;
            }

            if (toolbarBtn) {
                if (qtSettings.app == GTK_APP_GIMP &&
                    opts.focus == FOCUS_GLOW && toolbarBtn) {
                    x -= 2;
                    width += 4;
                    y -= 1;
                    height += 2;
                }
            }
#if 0 /* Removed in 1.7.2 */
            else {
                if (opts.focus == FOCUS_LINE) {
                    height--;
                } else if (oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED)) {
                    y--;
                    x -= 3;
                    width += 6;
                    height += 2;
                    if (!doEtch) {
                        y--;
                        x--;
                        width += 2;
                        height += 2;
                    }
                }
            }
#endif
        } else if (opts.focus == FOCUS_GLOW && toolbarBtn) {
            x -= 2;
            width += 4;
            y -= 2;
            height += 4;
        } else {
            if (doEtch) {
                x--;
                width += 2;
            } else {
                x -= 2;
                width += 4;
            }
            if (doEtch && (opts.thin & THIN_BUTTONS)) {
                y++;
                height -= 2;
            }
            btn = true;
        }
    }
    if (state == GTK_STATE_PRELIGHT && opts.coloredMouseOver != MO_NONE &&
        oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) && !listViewHeader &&
        (btn || comboButton)) {
        return;
    }
    if (opts.focus == FOCUS_GLOW && !comboButton && !listViewHeader &&
        !toolbarBtn && (btn || GTK_IS_SCALE(widget))) {
        return;
    }
    if (opts.focus == FOCUS_GLOW && toolbarBtn && state != GTK_STATE_NORMAL) {
        return;
    }
    if (opts.focus == FOCUS_STANDARD) {
        parent_class->draw_focus(style, window, state, area, widget, _detail,
                                 x, y, width, height);
    } else {
        bool drawRounded = (opts.round != ROUND_NONE);
        GdkColor *col =
            (view && state == GTK_STATE_SELECTED ? &style->text[state] :
             &qtcPalette.focus[FOCUS_SHADE(state == GTK_STATE_SELECTED)]);

        cairo_t *cr = Cairo::gdkCreateClip(window, area);

        if (qtSettings.app == GTK_APP_JAVA_SWT && view && widget &&
            GTK_IS_TREE_VIEW(widget)) {
            GtkTreeView *treeView = GTK_TREE_VIEW(widget);
            GtkTreePath *path = nullptr;
            GtkTreeViewColumn *column = nullptr;
            GtkTreeViewColumn *expanderColumn =
                gtk_tree_view_get_expander_column(treeView);

            TreeView::getCell(treeView, &path, &column, x, y, width, height);
            if (column == expanderColumn) {
                int expanderSize = 0;
                gtk_widget_style_get(widget, "expander-size",
                                     &expanderSize, nullptr);
                if (expanderSize > 0) {
                    int depth = path ? (int)gtk_tree_path_get_depth(path) : 0;
                    int offset =
                        (3 + expanderSize * depth +
                         (4 + gtk_tree_view_get_level_indentation(treeView)) *
                         (depth - 1));
                    x += offset;
                    width -= offset;
                }
            }

            if (path) {
                gtk_tree_path_free(path);
            }
        }

        if (oneOf(opts.focus, FOCUS_LINE, FOCUS_GLOW)) {
            if (view || listViewHeader) {
                height -= 2;
            }
            drawFadedLine(cr, x, y + height - 1, width, 1, col,
                          (QtcRect*)area, nullptr, true, true, true);
        } else {
            /* double alpha = (opts.focus == FOCUS_GLOW ? */
            /*                 FOCUS_GLOW_LINE_ALPHA : 1.0); */

            if (width < 3 || height < 3) {
                drawRounded = false;
            }
            cairo_new_path(cr);

            if (isListViewHeader(widget)) {
                btn = false;
                y++;
                x++;
                width -= 2;
                height -= 3;
            }
            if (oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED)) {
                if (btn) {
                    if (toolbarBtn) {
                        x -= 2;
                        y -= 2;
                        width += 4;
                        height += 4;
                        if (!doEtch) {
                            x -= 2;
                            width += 4;
                            y--;
                            height += 2;
                        }
                    } else if (!widget  || !(GTK_IS_RADIO_BUTTON(widget) ||
                                             GTK_IS_CHECK_BUTTON(widget))) {
                        /* 1.7.2 - dont asjust fot check/radio */
                        x -= 3;
                        y -= 3;
                        width += 6;
                        height += 6;
                    }
                }

                if (opts.focus == FOCUS_FILLED) {
                    if (drawRounded) {
                        Cairo::pathWhole(cr, x + 0.5, y + 0.5, width - 1,
                                         height - 1,
                                         qtcGetRadius(&opts, width, height,
                                                      WIDGET_OTHER,
                                                      RADIUS_EXTERNAL),
                                         comboButton ?
                                         (rev ? ROUNDED_LEFT : ROUNDED_RIGHT) :
                                         ROUNDED_ALL);
                    } else {
                        cairo_rectangle(cr, x + 0.5, y + 0.5,
                                        width - 1, height - 1);
                    }
                    Cairo::setColor(cr, col, FOCUS_ALPHA);
                    cairo_fill(cr);
                    cairo_new_path(cr);
                }
            }
            if (drawRounded) {
                Cairo::pathWhole(cr, x + 0.5, y + 0.5, width - 1, height - 1,
                                 (view &&
                                  opts.square & SQUARE_LISTVIEW_SELECTION) &&
                                 opts.round != ROUND_NONE ? SLIGHT_INNER_RADIUS :
                                 qtcGetRadius(&opts, width, height, WIDGET_OTHER,
                                              oneOf(opts.focus, FOCUS_FULL,
                                                    FOCUS_FILLED) ?
                                              RADIUS_EXTERNAL :
                                              RADIUS_SELECTION),
                                 oneOf(opts.focus, FOCUS_FULL,
                                       FOCUS_FILLED) && comboButton ?
                                 (rev ? ROUNDED_LEFT : ROUNDED_RIGHT) :
                                 ROUNDED_ALL);
            } else {
                cairo_rectangle(cr, x+0.5, y+0.5, width-1, height-1);
            }
            /* Cairo::setColor(cr, col, alpha); */
            Cairo::setColor(cr, col);
            cairo_stroke(cr);
        }
        cairo_destroy(cr);
    }
}

static void
gtkDrawResizeGrip(GtkStyle *style, GdkWindow *window, GtkStateType state,
                  GdkRectangle *_area, GtkWidget *widget, const char *_detail,
                  GdkWindowEdge edge, int x, int y, int width, int height)
{
    QTC_RET_IF_FAIL(GTK_IS_STYLE(style));
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    QtcRect *area = (QtcRect*)_area;
    cairo_t *cr = Cairo::gdkCreateClip(window, area);
    int size = SIZE_GRIP_SIZE - 2;

    /* Clear background */
    if (qtcIsFlatBgnd(opts.bgndAppearance) ||
        !(widget && drawWindowBgnd(cr, style, area, window, widget,
                                   x, y, width, height))) {
        if (widget && opts.bgndImage.type != IMG_NONE) {
            drawWindowBgnd(cr, style, area, window, widget,
                           x, y, width, height);
        }
    }

    switch (edge) {
    case GDK_WINDOW_EDGE_SOUTH_EAST: {
        // Adjust Firefox's resize grip so that it can be completely covered
        // by QtCurve's KWin resize grip.
        if (isMozilla()) {
            x++;
            y++;
        }
        const GdkPoint a[] = {{x + width, y + height - size},
                              {x + width, y + height},
                              {x + width - size,  y + height}};
        Cairo::polygon(cr, &qtcPalette.background[2], area, a, 3, true);
        break;
    }
    case GDK_WINDOW_EDGE_SOUTH_WEST: {
        const GdkPoint a[] = {{x + width - size, y + height - size},
                              {x + width, y + height},
                              {x + width - size, y + height}};
        Cairo::polygon(cr, &qtcPalette.background[2], area, a, 3, true);
        break;
    }
    case GDK_WINDOW_EDGE_NORTH_EAST:
        // TODO!!
    case GDK_WINDOW_EDGE_NORTH_WEST:
        // TODO!!
    default:
        parent_class->draw_resize_grip(style, window, state, _area, widget,
                                       _detail, edge, x, y, width, height);
    }
    cairo_destroy(cr);
}

static void
gtkDrawExpander(GtkStyle *style, GdkWindow *window, GtkStateType state,
                GdkRectangle *_area, GtkWidget *widget, const char *_detail,
                int x, int y, GtkExpanderStyle expander_style)
{
    QTC_RET_IF_FAIL(GDK_IS_DRAWABLE(window));
    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %s  ", __FUNCTION__, state, _detail);
        debugDisplayWidget(widget, 10);
    }
    QtcRect *area = (QtcRect*)_area;
    cairo_t *cr = gdk_cairo_create(window);
    bool isExpander = widget && (GTK_IS_EXPANDER(widget) ||
                                 GTK_IS_TREE_VIEW(widget));
    bool fill = (!isExpander || opts.coloredMouseOver ||
                 state != GTK_STATE_PRELIGHT);
    const GdkColor *col = (isExpander && opts.coloredMouseOver &&
                           state == GTK_STATE_PRELIGHT ?
                           &qtcPalette.mouseover[ARROW_MO_SHADE] :
                           &style->text[ARROW_STATE(state)]);

    x -= LV_SIZE / 2.0 + 0.5;
    x += 2;
    y -= LV_SIZE / 2.0 + 0.5;

    if (expander_style == GTK_EXPANDER_COLLAPSED) {
        Cairo::arrow(cr, col, area,
                     reverseLayout(widget) ? GTK_ARROW_LEFT : GTK_ARROW_RIGHT,
                     x + LARGE_ARR_WIDTH / 2, y + LARGE_ARR_HEIGHT,
                     false, fill, opts.vArrows);
    } else {
        Cairo::arrow(cr, col, area, GTK_ARROW_DOWN, x + LARGE_ARR_WIDTH / 2,
                     y + LARGE_ARR_HEIGHT, false, fill, opts.vArrows);
    }
    cairo_destroy(cr);
}

static void
styleRealize(GtkStyle *style)
{
    Style *qtc_style = (Style*)style;

    parent_class->realize(style);

    qtc_style->button_text[PAL_ACTIVE] =
        &qtSettings.colors[PAL_ACTIVE][COLOR_BUTTON_TEXT];
    qtc_style->button_text[PAL_DISABLED] =
        (qtSettings.qt4 ? &qtSettings.colors[PAL_DISABLED][COLOR_BUTTON_TEXT] :
         &style->text[GTK_STATE_INSENSITIVE]);

    if (opts.shadeMenubars == SHADE_WINDOW_BORDER) {
        qtc_style->menutext[0] =
            &qtSettings.colors[PAL_INACTIVE][COLOR_WINDOW_BORDER_TEXT];
        qtc_style->menutext[1] =
            &qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER_TEXT];
    } else if (opts.customMenuTextColor) {
        qtc_style->menutext[0] = &opts.customMenuNormTextColor;
        qtc_style->menutext[1] = &opts.customMenuSelTextColor;
    } else {
        qtc_style->menutext[0] = nullptr;
    }
}

static void
styleUnrealize(GtkStyle *style)
{
    parent_class->unrealize(style);
}

static void
style_init_from_rc(GtkStyle *style, GtkRcStyle *rc_style)
{
    parent_class->init_from_rc(style, rc_style);
}

static void
style_class_init(StyleClass *klass)
{
    GtkStyleClass *style_class = GTK_STYLE_CLASS(klass);

    parent_class = (GtkStyleClass*)g_type_class_peek_parent(klass);

    style_class->realize = styleRealize;
    style_class->unrealize = styleUnrealize;
    style_class->init_from_rc = style_init_from_rc;
    style_class->draw_resize_grip = gtkDrawResizeGrip;
    style_class->draw_expander = gtkDrawExpander;
    style_class->draw_arrow = gtkDrawArrow;
    style_class->draw_tab = gtkDrawTab;
    style_class->draw_shadow = gtkDrawShadow;
    style_class->draw_box_gap = gtkDrawBoxGap;
    style_class->draw_extension = gtkDrawExtension;
    style_class->draw_handle = gtkDrawHandle;
    style_class->draw_box = gtkDrawBox;
    style_class->draw_flat_box = gtkDrawFlatBox;
    style_class->draw_check = gtkDrawCheck;
    style_class->draw_slider = gtkDrawSlider;
    style_class->draw_option = gtkDrawOption;
    style_class->draw_shadow_gap = gtkDrawShadowGap;
    style_class->draw_hline = gtkDrawHLine;
    style_class->draw_vline = gtkDrawVLine;
    style_class->draw_focus = gtkDrawFocus;
    style_class->draw_layout = gtkDrawLayout;
    style_class->render_icon = gtkRenderIcon;
}

static GtkRcStyleClass *parent_rc_class;

static unsigned
rc_style_parse(GtkRcStyle*, GtkSettings*, GScanner *scanner)
{
    static GQuark scope_id = 0;
    unsigned old_scope;
    unsigned token;

    /* Set up a new scope in this scanner. */
    if (!scope_id)
        scope_id = g_quark_from_string("qtcurve_theme_engine");

    /* If we bail out due to errors, we *don't* reset the scope, so the error messaging code can make
       sense of our tokens. */
    old_scope = g_scanner_set_scope(scanner, scope_id);

    token = g_scanner_peek_next_token(scanner);
    while(token != G_TOKEN_RIGHT_CURLY)
    {
        switch(token)
        {
            default:
                g_scanner_get_next_token(scanner);
                token = G_TOKEN_RIGHT_CURLY;
        }

        if(token != G_TOKEN_NONE)
            return token;

        token = g_scanner_peek_next_token(scanner);
    }

    g_scanner_get_next_token(scanner);
    g_scanner_set_scope(scanner, old_scope);

    return G_TOKEN_NONE;
}

static void
rc_style_merge(GtkRcStyle *dest, GtkRcStyle *src)
{

    GtkRcStyle copy;
    const char *typeName=src ? g_type_name(G_TYPE_FROM_INSTANCE(src)) : nullptr;
    bool destIsQtc = isRcStyle(dest),
        srcIsQtc=!src->name || qtcStrStartsWith(src->name, RC_SETTING) ||
        qtcStrStartsWith(src->name, qtcGetProgName()),
        isQtCNoteBook=0!=opts.tabBgnd && src->name && 0==strcmp(src->name, "qtcurve-notebook_bg"),
                dontChangeColors=destIsQtc && !srcIsQtc && !isQtCNoteBook &&
                                 // Only allow GtkRcStyle and QtCurve::RcStyle to change colours
                                 // ...this should catch most cases whre another themes gtkrc is in the
                                 // GTK2_RC_FILES path
                                ( (typeName && strcmp(typeName, "GtkRcStyle") && strcmp(typeName, "QtCurveRcStyle")) ||
                                 // If run as root (probably via kdesu/kdesudo) then dont allow KDE settings to take
                                 // effect - as these are sometimes from the user's settings, not roots!
                                  (0==getuid() && src && src->name && (0==strcmp(src->name, "ToolTip") ||
                                                                      0==strcmp(src->name, "default"))));

    if (isQtCNoteBook) {
        qtcShade(&qtcPalette.background[ORIGINAL_SHADE],
                 &src->bg[GTK_STATE_NORMAL], TO_FACTOR(opts.tabBgnd),
                 opts.shading);
    }

    if(dontChangeColors)
    {
        memcpy(copy.color_flags, dest->color_flags, sizeof(GtkRcFlags)*5);
        memcpy(copy.fg, dest->fg, sizeof(GdkColor)*5);
        memcpy(copy.bg, dest->bg, sizeof(GdkColor)*5);
        memcpy(copy.text, dest->text, sizeof(GdkColor)*5);
        memcpy(copy.base, dest->base, sizeof(GdkColor)*5);
    }

    parent_rc_class->merge(dest, src);

    if(dontChangeColors)
    {
        memcpy(dest->color_flags, copy.color_flags, sizeof(GtkRcFlags)*5);
        memcpy(dest->fg, copy.fg, sizeof(GdkColor)*5);
        memcpy(dest->bg, copy.bg, sizeof(GdkColor)*5);
        memcpy(dest->text, copy.text, sizeof(GdkColor)*5);
        memcpy(dest->base, copy.base, sizeof(GdkColor)*5);
    }
}

/* Create an empty style suitable to this RC style */
static GtkStyle*
rc_style_create_style(GtkRcStyle *rc_style)
{
    GtkStyle *style = (GtkStyle*)g_object_new(style_type, nullptr);

    qtSettingsSetColors(style, rc_style);
    return style;
}

static void style_init(Style*)
{
    Shadow::initialize();
}

static void
style_register_type(GTypeModule *module)
{
    static const GTypeInfo object_info = {
        sizeof(StyleClass),
        (GBaseInitFunc)nullptr,
        (GBaseFinalizeFunc)nullptr,
        (GClassInitFunc)style_class_init,
        nullptr, /* class_finalize */
        nullptr, /* class_data */
        sizeof(Style),
        0, /* n_preallocs */
        (GInstanceInitFunc)style_init,
        nullptr
    };

    style_type = g_type_module_register_type(module, GTK_TYPE_STYLE,
                                             "QtCurveStyle", &object_info,
                                             GTypeFlags(0));
}

static gboolean
style_set_hook(GSignalInvocationHint*, unsigned, const GValue *argv, void*)
{
    GtkWidget *widget = GTK_WIDGET(g_value_get_object(argv));
    GdkScreen *screen = gtk_widget_get_screen(widget);
    if (!screen) {
        return true;
    }
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkVisual *visual = nullptr;
    if (gtk_widget_is_toplevel(widget)) {
        visual = gdk_screen_get_rgba_visual(screen);
    } else if (GTK_IS_DRAWING_AREA(widget)) {
        visual = gdk_screen_get_system_visual(screen);
    }
    if (visual) {
        gtk_widget_set_visual(widget, visual);
    }
#else
    GdkColormap *colormap = nullptr;
    if (gtk_widget_is_toplevel(widget)) {
        colormap = gdk_screen_get_rgba_colormap(screen);
    } else if (GTK_IS_DRAWING_AREA(widget)) {
        // For inkscape and flash (and probably others).
        // Inkscape's (at least the Gtk2 version) EekPreview widget
        // uses system default colormap instead of the one for the widget.
        // Not sure what's wrong with flash...
        colormap = gdk_screen_get_default_colormap(screen);
    }
    if (colormap) {
        gtk_widget_set_colormap(widget, colormap);
    }
#endif
    return true;
}

static void
rc_style_init(RcStyle*)
{
#ifdef INCREASE_SB_SLIDER
    lastSlider.widget = nullptr;
#endif
    if (qtSettingsInit()) {
        generateColors();
        if (qtSettings.useAlpha) {
            // Somehow GtkWidget is not loaded yet
            // when this function is called.
            g_type_class_ref(GTK_TYPE_WIDGET);
            g_signal_add_emission_hook(
                g_signal_lookup("style-set", GTK_TYPE_WIDGET),
                0, style_set_hook, nullptr, nullptr);
        }
    }
}

static void
rc_style_finalize(GObject *object)
{
    Animation::cleanup();
    qtcCall(G_OBJECT_CLASS(parent_rc_class)->finalize, object);
}

static void
rc_style_class_init(RcStyleClass *klass)
{
    GtkRcStyleClass *rc_style_class = GTK_RC_STYLE_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    parent_rc_class = (GtkRcStyleClass*)g_type_class_peek_parent(klass);

    rc_style_class->parse = rc_style_parse;
    rc_style_class->create_style = rc_style_create_style;
    rc_style_class->merge = rc_style_merge;

    object_class->finalize = rc_style_finalize;
}

static void
rc_style_register_type(GTypeModule *module)
{
    static const GTypeInfo object_info = {
        sizeof(RcStyleClass),
        nullptr,
        nullptr,
        (GClassInitFunc)rc_style_class_init,
        nullptr, /* class_finalize */
        nullptr, /* class_data */
        sizeof(RcStyle),
        0, /* n_preallocs */
        (GInstanceInitFunc)rc_style_init,
        nullptr
    };

    rc_style_type = g_type_module_register_type(module, GTK_TYPE_RC_STYLE,
                                                "QtCurveRcStyle", &object_info,
                                                GTypeFlags(0));
}
}

QTC_BEGIN_DECLS

QTC_EXPORT void
theme_init(GTypeModule *module)
{
    qtcX11InitXlib(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()));
    QtCurve::rc_style_register_type(module);
    QtCurve::style_register_type(module);
}

QTC_EXPORT void
theme_exit()
{
}

QTC_EXPORT GtkRcStyle*
theme_create_rc_style()
{
    return GTK_RC_STYLE(g_object_new(QtCurve::rc_style_type, nullptr));
}

/* The following function will be called by GTK+ when the module is loaded and
 * checks to see if we are compatible with the version of GTK+ that loads us.
 */
QTC_EXPORT const char*
g_module_check_init(GModule*)
{
    return gtk_check_version(GTK_MAJOR_VERSION, GTK_MINOR_VERSION,
                             GTK_MICRO_VERSION - GTK_INTERFACE_AGE);
}

QTC_END_DECLS
