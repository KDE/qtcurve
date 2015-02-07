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

#include "drawing.h"


#include "qt_settings.h"
#include "helpers.h"
#include "pixcache.h"
#include "entry.h"
#include "tab.h"
#include "animation.h"

#include <qtcurve-utils/gtkprops.h>
#include <qtcurve-utils/color.h>
#include <qtcurve-utils/log.h>

#include <common/config_file.h>

#if GTK_CHECK_VERSION(2, 90, 0)
static cairo_region_t*
windowMask(int x, int y, int w, int h, bool full)
{
    QtcRect rects[4];
    int numRects = 4;

    if (full) {
        rects[0] = qtcRect(x + 4, y + 0, w - 4 * 2, h);
        rects[1] = qtcRect(x + 0, y + 4, w, h - 4 * 2);
        rects[2] = qtcRect(x + 2, y + 1, w - 2 * 2, h - 2);
        rects[3] = qtcRect(x + 1, y + 2, w - 2, h - 2 * 2);
    } else {
        rects[0] = qtcRect(x + 1, y + 1, w - 2, h - 2);
        rects[1] = qtcRect(x, y + 2, w, h - 4);
        rects[2] = qtcRect(x + 2, y, w - 4, h);
        numRects = 3;
    }
    return cairo_region_create_rectangles(rects, numRects);
}
#endif

void
drawBgnd(cairo_t *cr, const GdkColor *col, GtkWidget *widget,
         const QtcRect *area, int x, int y, int width, int height)
{
    qtcCairoRect(cr, area, x, y, width, height,
                 qtcDefault(getParentBgCol(widget), col));
}

void
drawAreaModColor(cairo_t *cr, const QtcRect *area, const GdkColor *orig,
                 double mod, int x, int y, int width, int height)
{
    const GdkColor modified = shadeColor(orig, mod);
    qtcCairoRect(cr, area, x, y, width, height, &modified);
}

void
drawBevelGradient(cairo_t *cr, const QtcRect *area, int x, int y,
                  int width, int height, const GdkColor *base, bool horiz,
                  bool sel, EAppearance bevApp, EWidget w, double alpha)
{
    /* EAppearance app = ((APPEARANCE_BEVELLED != bevApp || WIDGET_BUTTON(w) || */
    /*                     WIDGET_LISTVIEW_HEADER == w) ? bevApp : */
    /*                    APPEARANCE_GRADIENT); */

    if (qtcIsFlat(bevApp)) {
        if (qtcNoneOf(w, WIDGET_TAB_TOP, WIDGET_TAB_BOT) ||
            !qtcIsCustomBgnd(&opts) || opts.tabBgnd || !sel) {
            qtcCairoRect(cr, area, x, y, width, height, base, alpha);
        }
    } else {
        cairo_pattern_t *pt =
            cairo_pattern_create_linear(x, y, horiz ? x : x + width - 1,
                                        horiz ? y + height - 1 : y);
        bool topTab = w == WIDGET_TAB_TOP;
        bool botTab = w == WIDGET_TAB_BOT;
        bool selected = (topTab || botTab) ? false : sel;
        EAppearance app = (selected ? opts.sunkenAppearance :
                           WIDGET_LISTVIEW_HEADER == w &&
                           APPEARANCE_BEVELLED == bevApp ?
                           APPEARANCE_LV_BEVELLED :
                           APPEARANCE_BEVELLED != bevApp ||
                           WIDGET_BUTTON(w) ||
                           WIDGET_LISTVIEW_HEADER == w ? bevApp :
                           APPEARANCE_GRADIENT);
        const Gradient *grad = qtcGetGradient(app, &opts);
        cairo_save(cr);
        qtcCairoClipRect(cr, area);

        for (int i = 0;i < grad->numStops;i++) {
            GdkColor col;
            double pos = botTab ? 1.0 - grad->stops[i].pos : grad->stops[i].pos;

            if ((topTab || botTab) && i == grad->numStops - 1) {
                if (sel && opts.tabBgnd == 0 && !isMozilla()) {
                    alpha = 0.0;
                }
                col = *base;
            } else {
                double val = (botTab && opts.invertBotTab ?
                              INVERT_SHADE(grad->stops[i].val) :
                              grad->stops[i].val);
                qtcShade(base, &col, botTab && opts.invertBotTab ?
                         qtcMax(val, 0.9) : val, opts.shading);
            }

            qtcCairoPatternAddColorStop(pt, pos, &col,
                                        qtcOneOf(w, WIDGET_TOOLTIP,
                                                 WIDGET_LISTVIEW_HEADER) ?
                                        alpha : alpha * grad->stops[i].alpha);
        }

        if (app == APPEARANCE_AGUA && !(topTab || botTab) &&
            (horiz ? height : width) > AGUA_MAX) {
            GdkColor col;
            double pos = AGUA_MAX / ((horiz ? height : width) * 2.0);

            qtcShade(base, &col, AGUA_MID_SHADE, opts.shading);
            qtcCairoPatternAddColorStop(pt, pos, &col, alpha);
            /* *grad->stops[i].alpha); */
            qtcCairoPatternAddColorStop(pt, 1.0 - pos, &col, alpha);
            /* *grad->stops[i].alpha); */
        }

        cairo_set_source(cr, pt);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
        cairo_pattern_destroy(pt);
        cairo_restore(cr);
    }
}

void
drawBorder(cairo_t *cr, GtkStyle *style, GtkStateType state,
           const QtcRect *area, int x, int y, int width, int height,
           const GdkColor *c_colors, ECornerBits round, EBorder borderProfile,
           EWidget widget, int flags, int borderVal)
{
    if (opts.round == ROUND_NONE && widget != WIDGET_RADIO_BUTTON) {
        round = ROUNDED_NONE;
    }
    double radius = qtcGetRadius(&opts, width, height, widget, RADIUS_EXTERNAL);
    double xd = x + 0.5;
    double yd = y + 0.5;
    /* EAppearance app = qtcWidgetApp(widget, &opts); */
    bool enabled = state != GTK_STATE_INSENSITIVE;
    bool useText = (enabled && widget == WIDGET_DEF_BUTTON &&
                    opts.defBtnIndicator == IND_FONT_COLOR);
    /* CPD USED TO INDICATE FOCUS! */
    bool hasFocus = (enabled && qtcPalette.focus &&
                     c_colors == qtcPalette.focus);
    bool hasMouseOver = (enabled && qtcPalette.mouseover &&
                         c_colors == qtcPalette.mouseover &&
                         opts.unifyCombo && opts.unifySpin);
    const GdkColor *colors = c_colors ? c_colors : qtcPalette.background;
    int useBorderVal = (!enabled && WIDGET_BUTTON(widget) ?
                        QTC_DISABLED_BORDER :
                        qtcPalette.mouseover == colors && IS_SLIDER(widget) ?
                        SLIDER_MO_BORDER_VAL : borderVal);
    const GdkColor *border_col = (useText ? &style->text[GTK_STATE_NORMAL] :
                                  &colors[useBorderVal]);
    width--;
    height--;
    cairo_save(cr);
    qtcCairoClipRect(cr, area);

    if (qtcOneOf(widget, WIDGET_TAB_BOT, WIDGET_TAB_TOP)) {
        colors = qtcPalette.background;
    }
    if (!(opts.thin&THIN_FRAMES)) {
        switch (borderProfile) {
        case BORDER_FLAT:
            break;
        case BORDER_RAISED:
        case BORDER_SUNKEN:
        case BORDER_LIGHT: {
            double radiusi = qtcGetRadius(&opts, width - 2, height - 2,
                                          widget, RADIUS_INTERNAL);
            double xdi = xd + 1;
            double ydi = yd + 1;
            double alpha = ((hasMouseOver || hasFocus) &&
                            qtcOneOf(widget, WIDGET_ENTRY, WIDGET_SPIN,
                                     WIDGET_COMBO_BUTTON) ? ENTRY_INNER_ALPHA :
                            BORDER_BLEND_ALPHA(widget));
            int widthi = width - 2;
            int heighti = height - 2;

            if ((state != GTK_STATE_INSENSITIVE ||
                 borderProfile == BORDER_SUNKEN) /* && */
                /* (qtcOneOf(borderProfile, BORDER_RAISED, BORDER_LIGHT) || */
                /*  app != APPEARANCE_FLAT) */) {
                const GdkColor *col =
                    &colors[qtcOneOf(borderProfile, BORDER_RAISED,
                                     BORDER_LIGHT) ? 0 : FRAME_DARK_SHADOW];
                if (flags & DF_BLEND) {
                    if (qtcOneOf(widget, WIDGET_SPIN, WIDGET_COMBO_BUTTON,
                                 WIDGET_SCROLLVIEW)) {
                        qtcCairoSetColor(cr, &style->base[state]);
                        qtcCairoPathTopLeft(cr, xdi, ydi, widthi, heighti,
                                            radiusi, round);
                        cairo_stroke(cr);
                    }
                    qtcCairoSetColor(cr, col, alpha);
                } else {
                    qtcCairoSetColor(cr, col);
                }
            } else {
                qtcCairoSetColor(cr, &style->bg[state]);
            }

            qtcCairoPathTopLeft(cr, xdi, ydi, widthi, heighti, radiusi, round);
            cairo_stroke(cr);
            if (widget != WIDGET_CHECKBOX) {
                if(!hasFocus && !hasMouseOver &&
                   borderProfile != BORDER_LIGHT) {
                    if (widget == WIDGET_SCROLLVIEW) {
                        /* Because of list view headers, need to draw dark
                         * line on right! */
                        cairo_save(cr);
                        qtcCairoSetColor(cr, &style->base[state]);
                        qtcCairoPathBottomRight(cr, xdi, ydi, widthi, heighti,
                                                radiusi, round);
                        cairo_stroke(cr);
                        cairo_restore(cr);
                    } else if (qtcOneOf(widget, WIDGET_SCROLLVIEW,
                                        WIDGET_ENTRY)) {
                        qtcCairoSetColor(cr, &style->base[state]);
                    } else if (state != GTK_STATE_INSENSITIVE &&
                               (borderProfile == BORDER_SUNKEN ||
                                /* app != APPEARANCE_FLAT ||*/
                                qtcOneOf(widget, WIDGET_TAB_TOP,
                                         WIDGET_TAB_BOT))) {
                        const GdkColor *col =
                            &colors[borderProfile == BORDER_RAISED ?
                                    FRAME_DARK_SHADOW : 0];
                        if (flags & DF_BLEND) {
                            qtcCairoSetColor(cr, col,
                                             borderProfile == BORDER_SUNKEN ?
                                             0.0 : alpha);
                        } else {
                            qtcCairoSetColor(cr, col);
                        }
                    } else {
                        qtcCairoSetColor(cr, &style->bg[state]);
                    }
                }

                qtcCairoPathBottomRight(cr, xdi, ydi, widthi, heighti,
                                        radiusi, round);
                cairo_stroke(cr);
            }
        }
        }
    }

    if (borderProfile == BORDER_SUNKEN &&
        (widget == WIDGET_FRAME ||
         (qtcOneOf(widget, WIDGET_ENTRY, WIDGET_SCROLLVIEW) &&
          !opts.etchEntry && !hasFocus && !hasMouseOver))) {
        qtcCairoSetColor(cr, border_col,
                         /*enabled ? */1.0/* : LOWER_BORDER_ALPHA*/);
        qtcCairoPathTopLeft(cr, xd, yd, width, height, radius, round);
        cairo_stroke(cr);
        qtcCairoSetColor(cr, border_col, LOWER_BORDER_ALPHA);
        qtcCairoPathBottomRight(cr, xd, yd, width, height, radius, round);
        cairo_stroke(cr);
    } else {
        qtcCairoSetColor(cr, border_col);
        qtcCairoPathWhole(cr, xd, yd, width, height, radius, round);
        cairo_stroke(cr);
    }
    cairo_restore(cr);
}

void
drawGlow(cairo_t *cr, const QtcRect *area, int x, int y, int w, int h,
         ECornerBits round, EWidget widget, const GdkColor *colors)
{
    if (qtcPalette.mouseover || qtcPalette.defbtn || colors) {
        double xd = x + 0.5;
        double yd = y + 0.5;
        double radius = qtcGetRadius(&opts, w, h, widget, RADIUS_ETCH);
        bool def = (widget == WIDGET_DEF_BUTTON &&
                    opts.defBtnIndicator == IND_GLOW);
        bool defShade =
            (def && (!qtcPalette.defbtn ||
                     (qtcPalette.mouseover &&
                      EQUAL_COLOR(qtcPalette.defbtn[ORIGINAL_SHADE],
                                  qtcPalette.mouseover[ORIGINAL_SHADE]))));
        const GdkColor *col =
            (colors ? &colors[GLOW_MO] : (def && qtcPalette.defbtn) ||
             !qtcPalette.mouseover ? &qtcPalette.defbtn[GLOW_DEFBTN] :
             &qtcPalette.mouseover[GLOW_MO]);

        cairo_save(cr);
        qtcCairoClipRect(cr, area);
        qtcCairoSetColor(cr, col, GLOW_ALPHA(defShade));
        qtcCairoPathWhole(cr, xd, yd, w - 1, h - 1, radius, round);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
}

void
drawEtch(cairo_t *cr, const QtcRect *area, GtkWidget *widget, int x, int y,
         int w, int h, bool raised, ECornerBits round, EWidget wid)
{
    double xd = x + 0.5;
    double yd = y + 0.5;
    double radius = qtcGetRadius(&opts, w, h, wid, RADIUS_ETCH);
    const QtcRect *a = area;
    QtcRect b;

    if (wid == WIDGET_TOOLBAR_BUTTON && opts.tbarBtnEffect == EFFECT_ETCH)
        raised = false;

    if (wid == WIDGET_COMBO_BUTTON && qtSettings.app == GTK_APP_OPEN_OFFICE &&
        widget && isFixedWidget(gtk_widget_get_parent(widget))) {
        b = qtcRect(x + 2, y, w - 4, h);
        a = &b;
    }
    cairo_save(cr);
    qtcCairoClipRect(cr, a);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0,
                          USE_CUSTOM_ALPHAS(opts) ?
                          opts.customAlphas[ALPHA_ETCH_DARK] : ETCH_TOP_ALPHA);
    if (!raised && wid != WIDGET_SLIDER) {
        qtcCairoPathTopLeft(cr, xd, yd, w - 1, h - 1, radius, round);
        cairo_stroke(cr);
        if (wid == WIDGET_SLIDER_TROUGH && opts.thinSbarGroove &&
            widget && GTK_IS_SCROLLBAR(widget)) {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0,
                                  USE_CUSTOM_ALPHAS(opts) ?
                                  opts.customAlphas[ALPHA_ETCH_LIGHT] :
                                  ETCH_BOTTOM_ALPHA);
        } else {
            setLowerEtchCol(cr, widget);
        }
    }
    qtcCairoPathBottomRight(cr, xd, yd, w - 1, h - 1, radius, round);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void
qtcClipPath(cairo_t *cr, int x, int y, int w, int h, EWidget widget,
            ERadius rad, ECornerBits round)
{
    qtcCairoClipWhole(cr, x + 0.5, y + 0.5, w - 1, h - 1,
                      qtcGetRadius(&opts, w, h, widget, rad), round);
}

QTC_ALWAYS_INLINE static inline void
addStripes(cairo_t *cr, int x, int y, int w, int h, bool horizontal)
{
    qtcCairoStripes(cr, x, y, w, h, horizontal, STRIPE_WIDTH);
}

void
drawLightBevel(cairo_t *cr, GtkStyle *style, GtkStateType state,
               const QtcRect *area, int x, int y, int width, int height,
               const GdkColor *base, const GdkColor *colors, ECornerBits round,
               EWidget widget, EBorder borderProfile, int flags, GtkWidget *wid)
{
    EAppearance app = qtcWidgetApp(APPEARANCE_NONE != opts.tbarBtnAppearance &&
                                   (WIDGET_TOOLBAR_BUTTON == widget ||
                                    (WIDGET_BUTTON(widget) &&
                                     isOnToolbar(wid, NULL, 0))) ?
                                   WIDGET_TOOLBAR_BUTTON : widget, &opts);
    bool sunken = flags & DF_SUNKEN;
    bool doColouredMouseOver =
        (opts.coloredMouseOver && qtcPalette.mouseover &&
         WIDGET_SPIN != widget && WIDGET_SPIN_DOWN != widget &&
         WIDGET_SPIN_UP != widget && WIDGET_COMBO_BUTTON != widget &&
         WIDGET_SB_BUTTON != widget && (!SLIDER(widget) ||
                                        !opts.colorSliderMouseOver) &&
         WIDGET_UNCOLOURED_MO_BUTTON != widget && GTK_STATE_PRELIGHT == state &&
         (!sunken || IS_TOGGLE_BUTTON(widget) ||
          (WIDGET_TOOLBAR_BUTTON == widget && opts.coloredTbarMo)));
    bool plastikMouseOver = (doColouredMouseOver &&
                             opts.coloredMouseOver == MO_PLASTIK);
    bool colouredMouseOver =
        (doColouredMouseOver && qtcOneOf(opts.coloredMouseOver, MO_COLORED,
                                         MO_COLORED_THICK));
    bool flatWidget = (widget == WIDGET_PROGRESSBAR && !opts.borderProgress);
    bool lightBorder = !flatWidget && DRAW_LIGHT_BORDER(sunken, widget, app);
    bool draw3dfull = (!flatWidget && !lightBorder &&
                       DRAW_3D_FULL_BORDER(sunken, app));
    bool draw3d =
        (!flatWidget && (draw3dfull ||
                         (!lightBorder && DRAW_3D_BORDER(sunken, app))));
    bool drawShine = DRAW_SHINE(sunken, app);
    bool bevelledButton = WIDGET_BUTTON(widget) && app == APPEARANCE_BEVELLED;
    bool doEtch =
        (flags & DF_DO_BORDER &&
         (ETCH_WIDGET(widget) || (WIDGET_COMBO_BUTTON == widget &&
                                  opts.etchEntry)) &&
         opts.buttonEffect != EFFECT_NONE);
    bool glowFocus =
        (doEtch && USE_GLOW_FOCUS(GTK_STATE_PRELIGHT == state) && wid &&
         ((flags&DF_HAS_FOCUS) || gtk_widget_has_focus(wid)) &&
         GTK_STATE_INSENSITIVE != state && !isComboBoxEntryButton(wid) &&
         ((WIDGET_RADIO_BUTTON != widget && WIDGET_CHECKBOX != widget) ||
          GTK_STATE_ACTIVE != state));
    bool glowFocusSunkenToggle =
        (sunken && (glowFocus || (doColouredMouseOver &&
                                  MO_GLOW == opts.coloredMouseOver)) &&
         wid && GTK_IS_TOGGLE_BUTTON(wid));
    bool horiz = !(flags & DF_VERT);
    int xe = x, ye = y;
    int we = width, he = height;
    int origWidth = width, origHeight = height;
    double xd = x + 0.5, yd = y + 0.5;
    if (CIRCULAR_SLIDER(widget))
        horiz = true;

    if (WIDGET_TROUGH == widget && !opts.borderSbarGroove &&
        flags & DF_DO_BORDER) {
        flags -= DF_DO_BORDER;
    }

    if (WIDGET_COMBO_BUTTON == widget && doEtch) {
        if (ROUNDED_RIGHT == round) {
            x--;
            xd -= 1;
            width++;
        } else if(ROUNDED_LEFT == round) {
            width++;
        }
    }

    if (doEtch) {
        xd += 1;
        x++;
        yd += 1;
        y++;
        width -= 2;
        height -= 2;
        xe = x;
        ye = y;
        we = width;
        he = height;
    }

    if (width > 0 && height > 0) {
        cairo_save(cr);
        if (!(flags & DF_DO_BORDER)) {
            qtcCairoClipWhole(cr, x, y, width, height,
                              qtcGetRadius(&opts, width, height, widget,
                                           RADIUS_EXTERNAL), round);
        } else {
            qtcClipPath(cr, x, y, width, height, widget,
                        RADIUS_EXTERNAL, round);
        }
        drawBevelGradient(cr, area, x, y, width, height, base, horiz,
                          sunken && !IS_TROUGH(widget), app, widget);
        if (plastikMouseOver) {
            if (SLIDER(widget)) {
                int len = SB_SLIDER_MO_LEN(horiz ? width : height);
                int so = lightBorder ? SLIDER_MO_PLASTIK_BORDER : 1;
                int eo = len + so;
                int col = SLIDER_MO_SHADE;

                if (horiz) {
                    drawBevelGradient(cr, area, x + so, y, len, height,
                                      &qtcPalette.mouseover[col], horiz,
                                      sunken, app, widget);
                    drawBevelGradient(cr, area, x + width - eo, y, len,
                                      height, &qtcPalette.mouseover[col],
                                      horiz, sunken, app, widget);
                } else {
                    drawBevelGradient(cr, area, x, y + so, width, len,
                                      &qtcPalette.mouseover[col], horiz,
                                      sunken, app, widget);
                    drawBevelGradient(cr, area, x, y + height - eo,
                                      width, len, &qtcPalette.mouseover[col],
                                      horiz, sunken, app, widget);
                }
            } else {
                int mh = height;
                const GdkColor *col =
                    &qtcPalette.mouseover[MO_PLASTIK_DARK(widget)];
                bool horizontal =
                    ((horiz && !(WIDGET_SB_BUTTON == widget ||
                                 SLIDER(widget))) ||
                     (!horiz && (WIDGET_SB_BUTTON == widget ||
                                 SLIDER(widget))));
                bool thin =
                    (qtcOneOf(widget, WIDGET_SB_BUTTON, WIDGET_SPIN_UP,
                              WIDGET_SPIN_DOWN) ||
                     (horiz ? height : width) < 16);

                if (EFFECT_NONE != opts.buttonEffect &&
                    WIDGET_SPIN_UP == widget && horiz) {
                    mh--;
                }
                cairo_new_path(cr);
                qtcCairoSetColor(cr, col);
                if (horizontal) {
                    cairo_move_to(cr, x + 1, yd + 1);
                    cairo_line_to(cr, x + width -1, yd + 1);
                    cairo_move_to(cr, x + 1, yd + mh - 2);
                    cairo_line_to(cr, x + width - 1, yd + mh - 2);
                } else {
                    cairo_move_to(cr, xd + 1, y + 1);
                    cairo_line_to(cr, xd + 1, y + mh - 1);
                    cairo_move_to(cr, xd + width - 2, y + 1);
                    cairo_line_to(cr, xd + width - 2, y + mh - 1);
                }
                cairo_stroke(cr);
                if (!thin) {
                    col = &qtcPalette.mouseover[MO_PLASTIK_LIGHT(widget)];
                    cairo_new_path(cr);
                    qtcCairoSetColor(cr, col);
                    if (horizontal) {
                        cairo_move_to(cr, x + 1, yd + 2);
                        cairo_line_to(cr, x + width - 1, yd + 2);
                        cairo_move_to(cr, x + 1, yd + mh - 3);
                        cairo_line_to(cr, x + width - 1, yd + mh - 3);
                    } else {
                        cairo_move_to(cr, xd + 2, y + 1);
                        cairo_line_to(cr, xd + 2, y + mh - 1);
                        cairo_move_to(cr, xd + width - 3, y + 1);
                        cairo_line_to(cr, xd + width - 3, y + mh - 1);
                    }
                    cairo_stroke(cr);
                }
            }
        }

        if (drawShine) {
            bool mo = state == GTK_STATE_PRELIGHT && opts.highlightFactor;
            int xa = x;
            int ya = y;
            int wa = width;
            int ha = height;
            if (widget == WIDGET_RADIO_BUTTON || CIRCULAR_SLIDER(widget)) {
                double topSize = ha * 0.4;
                double topWidthAdjust = 3.5;
                double topGradRectX = xa + topWidthAdjust;
                double topGradRectY = ya;
                double topGradRectW = wa - topWidthAdjust * 2 - 1;
                double topGradRectH = topSize - 1;
                cairo_pattern_t *pt =
                    cairo_pattern_create_linear(topGradRectX, topGradRectY,
                                                topGradRectX,
                                                topGradRectY + topGradRectH);

                cairo_save(cr);
                qtcCairoClipWhole(cr, topGradRectX + 0.5, topGradRectY + 0.5,
                                  topGradRectW, topGradRectH,
                                  topGradRectW / 2.0, ROUNDED_ALL);

                cairo_pattern_add_color_stop_rgba(
                    pt, 0.0, 1.0, 1.0, 1.0,
                    mo ? (opts.highlightFactor > 0 ? 0.8 : 0.7) : 0.75);
                cairo_pattern_add_color_stop_rgba(
                    pt, CAIRO_GRAD_END, 1.0, 1.0, 1.0,
                    /*mo ? (opts.highlightFactor>0 ? 0.3 : 0.1) :*/ 0.2);

                cairo_set_source(cr, pt);
                cairo_rectangle(cr, topGradRectX, topGradRectY, topGradRectW,
                                topGradRectH);
                cairo_fill(cr);
                cairo_pattern_destroy(pt);
                cairo_restore(cr);
            } else {
                double size = qtcMin((horiz ? ha : wa) / 2.0, 16);
                double rad = size / 2.0;
                cairo_pattern_t *pt = NULL;
                int mod = 4;

                if (horiz) {
                    if (!(ROUNDED_LEFT & round)) {
                        xa -= 8;
                        wa += 8;
                    }
                    if (!(ROUNDED_RIGHT & round)) {
                        wa += 8;
                    }
                } else {
                    if (!(ROUNDED_TOP & round)) {
                        ya -= 8;
                        ha += 8;
                    }
                    if (!(ROUNDED_BOTTOM & round)) {
                        ha += 8;
                    }
                }
                pt = cairo_pattern_create_linear(
                    xa, ya, xa + (horiz ? 0.0 : size),
                    ya + (horiz ? size : 0.0));

                if (qtcGetWidgetRound(&opts, origWidth, origHeight,
                                      widget) < ROUND_MAX ||
                    (!IS_MAX_ROUND_WIDGET(widget) && !IS_SLIDER(widget))) {
                    rad /= 2.0;
                    mod /= 2;
                }

                cairo_save(cr);
                if (horiz) {
                    qtcCairoClipWhole(cr, xa + mod + 0.5, ya + 0.5,
                                      wa - mod * 2 - 1, size - 1, rad, round);
                } else {
                    qtcCairoClipWhole(cr, xa + 0.5, ya + mod + 0.5, size - 1,
                                      ha - mod * 2 - 1, rad, round);
                }

                cairo_pattern_add_color_stop_rgba(
                    pt, 0.0, 1.0, 1.0, 1.0,
                    mo ? (opts.highlightFactor > 0 ? 0.95 : 0.85) : 0.9);
                cairo_pattern_add_color_stop_rgba(
                    pt, CAIRO_GRAD_END, 1.0, 1.0, 1.0,
                    mo ? (opts.highlightFactor > 0 ? 0.3 : 0.1) : 0.2);

                cairo_set_source(cr, pt);
                cairo_rectangle(cr, xa, ya, horiz ? wa : size,
                                horiz ? size : ha);
                cairo_fill(cr);
                cairo_pattern_destroy(pt);
                cairo_restore(cr);
            }
        }
        cairo_restore(cr);
    }
    xd += 1;
    x++;
    yd += 1;
    y++;
    width -= 2;
    height -= 2;

    cairo_save(cr);
    if (plastikMouseOver /* && !sunken */) {
        bool thin = (qtcOneOf(widget, WIDGET_SB_BUTTON, WIDGET_SPIN) ||
                     (horiz ? height : width) < 16);
        bool horizontal =
            (SLIDER(widget) ? !horiz : (horiz && WIDGET_SB_BUTTON != widget) ||
             (!horiz && WIDGET_SB_BUTTON == widget));
        int len = (SLIDER(widget) ? SB_SLIDER_MO_LEN(horiz ? width : height) :
                   (thin ? 1 : 2));
        QtcRect rect;
        if (horizontal) {
            rect = qtcRect(x, y + len, width, height - len * 2);
        } else {
            rect = qtcRect(x + len, y, width - len * 2, height);
        }
        qtcCairoClipRect(cr, &rect);
    } else {
        qtcCairoClipRect(cr, area);
    }

    if (!colouredMouseOver && lightBorder) {
        const GdkColor *col = &colors[LIGHT_BORDER(app)];

        cairo_new_path(cr);
        qtcCairoSetColor(cr, col);
        qtcCairoPathWhole(cr, xd, yd, width - 1, height - 1,
                          qtcGetRadius(&opts, width, height, widget,
                                       RADIUS_INTERNAL), round);
        cairo_stroke(cr);
    }  else if (colouredMouseOver ||
                (!sunken && (draw3d || flags & DF_DRAW_INSIDE))) {
        int dark = /*bevelledButton ? */2/* : 4*/;
        const GdkColor *col1 =
            (colouredMouseOver ?
             &qtcPalette.mouseover[MO_STD_LIGHT(widget, sunken)] :
             &colors[sunken ? dark : 0]);

        cairo_new_path(cr);
        qtcCairoSetColor(cr, col1);
        qtcCairoPathTopLeft(cr, xd, yd, width - 1, height - 1,
                            qtcGetRadius(&opts, width, height, widget,
                                         RADIUS_INTERNAL), round);
        cairo_stroke(cr);
        if (colouredMouseOver || bevelledButton || draw3dfull) {
            const GdkColor *col2 = colouredMouseOver ?
                &qtcPalette.mouseover[MO_STD_DARK(widget)] :
                &colors[sunken ? 0 : dark];

            cairo_new_path(cr);
            qtcCairoSetColor(cr, col2);
            qtcCairoPathBottomRight(cr, xd, yd, width - 1, height - 1,
                                    qtcGetRadius(&opts, width, height, widget,
                                                 RADIUS_INTERNAL), round);
            cairo_stroke(cr);
        }
    }

    cairo_restore(cr);

    if ((doEtch || glowFocus) && !(flags & DF_HIDE_EFFECT)) {
        if ((!sunken || glowFocusSunkenToggle) &&
            GTK_STATE_INSENSITIVE != state && !(opts.thin&THIN_FRAMES) &&
            ((WIDGET_OTHER != widget && WIDGET_SLIDER_TROUGH != widget &&
              WIDGET_COMBO_BUTTON != widget &&
              MO_GLOW == opts.coloredMouseOver &&
              GTK_STATE_PRELIGHT == state) || glowFocus ||
             (WIDGET_DEF_BUTTON == widget &&
              IND_GLOW == opts.defBtnIndicator))) {
            drawGlow(cr, area, xe - 1, ye - 1, we + 2, he + 2, round,
                     (widget == WIDGET_DEF_BUTTON &&
                      state == GTK_STATE_PRELIGHT ? WIDGET_STD_BUTTON : widget),
                     glowFocus ? qtcPalette.focus : NULL);
        } else {
            drawEtch(cr, area, wid,
                     xe - (WIDGET_COMBO_BUTTON == widget ?
                           (ROUNDED_RIGHT==round ? 3 : 1) : 1), ye - 1,
                     we + (WIDGET_COMBO_BUTTON == widget ?
                           (ROUNDED_RIGHT == round ? 4 : 5) : 2), he + 2,
                     EFFECT_SHADOW == opts.buttonEffect &&
                     WIDGET_COMBO_BUTTON != widget && WIDGET_BUTTON(widget) &&
                     !sunken, round, widget);
        }
    }

    xd -= 1;
    x--;
    yd -= 1;
    y--;
    width += 2;
    height += 2;
    if (flags & DF_DO_BORDER && width > 2 && height > 2) {
        const GdkColor *borderCols =
            (glowFocus && (!sunken || glowFocusSunkenToggle) ?
             qtcPalette.focus : qtcOneOf(widget, WIDGET_COMBO,
                                         WIDGET_COMBO_BUTTON) &&
             qtcPalette.combobtn == colors ?
             state == GTK_STATE_PRELIGHT && opts.coloredMouseOver == MO_GLOW &&
             !sunken ? qtcPalette.mouseover :
             qtcPalette.button[PAL_ACTIVE] : colors);

        cairo_new_path(cr);
        /* Yuck! this is a mess!!!! */
        // Copied from KDE4 version...
        if (!sunken && state != GTK_STATE_INSENSITIVE && !glowFocus &&
            ((((doEtch && qtcNoneOf(widget, WIDGET_OTHER,
                                    WIDGET_SLIDER_TROUGH)) || SLIDER(widget) ||
               qtcOneOf(widget, WIDGET_COMBO, WIDGET_MENU_BUTTON)) &&
              (opts.coloredMouseOver == MO_GLOW
               /* || opts.colorMenubarMouseOver == MO_COLORED */) &&
              state == GTK_STATE_PRELIGHT) ||
             (doEtch && widget == WIDGET_DEF_BUTTON &&
              opts.defBtnIndicator == IND_GLOW))) {

// Previous Gtk2...
//         if(!sunken && (doEtch || SLIDER(widget)) &&
//             ( (WIDGET_OTHER!=widget && WIDGET_SLIDER_TROUGH!=widget && WIDGET_COMBO_BUTTON!=widget &&
//                 MO_GLOW==opts.coloredMouseOver && GTK_STATE_PRELIGHT==state) ||
//               (WIDGET_DEF_BUTTON==widget && IND_GLOW==opts.defBtnIndicator)))
            drawBorder(cr, style, state, area, x, y, width, height,
                       widget == WIDGET_DEF_BUTTON &&
                       opts.defBtnIndicator == IND_GLOW &&
                       (state != GTK_STATE_PRELIGHT || !qtcPalette.mouseover) ?
                       qtcPalette.defbtn : qtcPalette.mouseover,
                       round, borderProfile, widget, flags);
        } else {
            drawBorder(cr, style, state, area, x, y, width, height,
                       colouredMouseOver &&
                       opts.coloredMouseOver == MO_COLORED_THICK ?
                       qtcPalette.mouseover : borderCols,
                       round, borderProfile, widget, flags);
        }
    }

    if (WIDGET_SB_SLIDER == widget && opts.stripedSbar) {
        cairo_save(cr);
        qtcCairoClipWhole(cr, x, y, width, height,
                          qtcGetRadius(&opts, width, height, WIDGET_SB_SLIDER,
                                       RADIUS_INTERNAL), round);
        addStripes(cr, x + 1, y + 1, width - 2, height - 2, horiz);
        cairo_restore(cr);
    }
}

void
drawFadedLine(cairo_t *cr, int x, int y, int width, int height,
              const GdkColor *col, const QtcRect *area, const QtcRect *gap,
              bool fadeStart, bool fadeEnd, bool horiz, double alpha)
{
    qtcCairoFadedLine(cr, x, y, width, height, area, gap,
                      fadeStart && opts.fadeLines,
                      fadeEnd && opts.fadeLines, FADE_SIZE, horiz, col, alpha);
}

void
drawHighlight(cairo_t *cr, int x, int y, int width, int height,
              const QtcRect *area, bool horiz, bool inc)
{
    drawFadedLine(cr, x, y, width, height,
                  &qtcPalette.mouseover[ORIGINAL_SHADE], area, NULL,
                  true, true, horiz, inc ? 0.5 : 1.0);
    drawFadedLine(cr, x + (horiz ? 0 : 1), y + (horiz ? 1 : 0), width, height,
                  &qtcPalette.mouseover[ORIGINAL_SHADE], area, NULL,
                  true, true, horiz, inc ? 1.0 : 0.5);
}

void setLineCol(cairo_t *cr, cairo_pattern_t *pt, const GdkColor *col)
{
    if (pt) {
        qtcCairoPatternAddColorStop(pt, 0, col, 0.0);
        qtcCairoPatternAddColorStop(pt, 0.4, col);
        qtcCairoPatternAddColorStop(pt, 0.6, col);
        qtcCairoPatternAddColorStop(pt, CAIRO_GRAD_END, col, 0.0);
        cairo_set_source(cr, pt);
    } else {
        qtcCairoSetColor(cr, col);
    }
}

void
drawLines(cairo_t *cr, double rx, double ry, int rwidth, int rheight,
          bool horiz, int nLines, int offset, const GdkColor *cols,
          const QtcRect *area, int dark, ELine type)
{
    if (horiz) {
        ry += 0.5;
        rwidth += 1;
    } else {
        rx += 0.5;
        rheight += 1;
    }
    int space = nLines * 2 + (type != LINE_DASHES ? nLines - 1 : 0);
    int step = type != LINE_DASHES ? 3 : 2;
    int etchedDisp = type == LINE_SUNKEN ? 1 : 0;
    double x = horiz ? rx : rx + (rwidth - space) / 2;
    double y = horiz ? ry + (rheight - space) / 2 : ry;
    double x2 = rx + rwidth - 1;
    double y2 = ry + rheight - 1;
    const GdkColor *col1 = &cols[dark];
    const GdkColor *col2 = &cols[0];
    cairo_pattern_t *pt1 =
        ((opts.fadeLines && (horiz ? rwidth : rheight) > (16 + etchedDisp)) ?
         cairo_pattern_create_linear(rx, ry, horiz ? x2 : rx + 1,
                                     horiz ? ry + 1 : y2) : NULL);
    cairo_pattern_t *pt2 =
        ((pt1 && type != LINE_FLAT) ?
         cairo_pattern_create_linear(rx, ry, horiz ? x2 : rx + 1,
                                     horiz ? ry + 1 : y2) : NULL);
    cairo_save(cr);
    qtcCairoClipRect(cr, area);
    setLineCol(cr, pt1, col1);
    if (horiz) {
        for (int i = 0;i < space;i += step) {
            cairo_move_to(cr, x + offset, y + i);
            cairo_line_to(cr, x2 - offset, y + i);
        }
        cairo_stroke(cr);
        if (type != LINE_FLAT) {
            setLineCol(cr, pt2, col2);
            x += etchedDisp;
            x2 += etchedDisp;
            for (int i = 1;i < space;i += step) {
                cairo_move_to(cr, x + offset, y + i);
                cairo_line_to(cr, x2 - offset, y + i);
            }
            cairo_stroke(cr);
        }
    } else {
        for (int i = 0;i < space;i += step) {
            cairo_move_to(cr, x + i, y + offset);
            cairo_line_to(cr, x + i, y2 - offset);
        }
        cairo_stroke(cr);
        if (type != LINE_FLAT) {
            setLineCol(cr, pt2, col2);
            y += etchedDisp;
            y2 += etchedDisp;
            for (int i = 1;i < space;i += step) {
                cairo_move_to(cr, x + i, y + offset);
                cairo_line_to(cr, x + i, y2 - offset);
            }
            cairo_stroke(cr);
        }
    }
    if (pt1) {
        cairo_pattern_destroy(pt1);
    }
    if (pt2) {
        cairo_pattern_destroy(pt2);
    }
    cairo_restore(cr);
}

void
drawEntryCorners(cairo_t *cr, const QtcRect *area, int round, int x, int y,
                 int width, int height, const GdkColor *col, double a)
{
    cairo_save(cr);
    qtcCairoClipRect(cr, area);
    qtcCairoSetColor(cr, col, a);
    cairo_rectangle(cr, x + 0.5, y + 0.5, width - 1, height - 1);
    if (opts.buttonEffect != EFFECT_NONE && opts.etchEntry) {
        cairo_rectangle(cr, x + 1.5, y + 1.5, width - 2, height - 3);
    }
    if (opts.round > ROUND_FULL) {
        if (round & CORNER_TL) {
            cairo_rectangle(cr, x + 2.5, y + 2.5, 1, 1);
        }
        if (round & CORNER_BL) {
            cairo_rectangle(cr, x + 2.5, y + height - 3.5, 1, 1);
        }
        if (round & CORNER_TR) {
            cairo_rectangle(cr, x + width - 2.5, y + 2.5, 1, 1);
        }
        if (round & CORNER_BR) {
            cairo_rectangle(cr, x + width - 2.5, y + height - 3.5, 1, 1);
        }
    }
    cairo_set_line_width(cr, opts.round > ROUND_FULL &&
                         qtSettings.app != GTK_APP_OPEN_OFFICE ? 2 : 1);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void
drawBgndRing(cairo_t *cr, int x, int y, int size, int size2, bool isWindow)
{
    double width = (size - size2) / 2.0;
    double width2 = width / 2.0;
    double radius = (size2 + width) / 2.0;

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0,
                          RINGS_INNER_ALPHA(isWindow ? opts.bgndImage.type :
                                            opts.menuBgndImage.type));
    cairo_set_line_width(cr, width);
    cairo_arc(cr, x + radius + width2 + 0.5, y + radius + width2 + 0.5, radius,
              0, 2 * M_PI);
    cairo_stroke(cr);

    if ((isWindow ? opts.bgndImage.type :
         opts.menuBgndImage.type) == IMG_BORDERED_RINGS) {
        cairo_set_line_width(cr, 1);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, RINGS_OUTER_ALPHA);
        cairo_arc(cr, x + radius + width2 + 0.5, y + radius + width2 + 0.5,
                  size / 2.0, 0, 2 * M_PI);
        if (size2) {
            cairo_stroke(cr);
            cairo_arc(cr, x + radius + width2 + 0.5, y + radius + width2 + 0.5,
                      size2 / 2.0, 0, 2 * M_PI);
        }
        cairo_stroke(cr);
    }
}

void
drawBgndRings(cairo_t *cr, int x, int y, int width, int height, bool isWindow)
{
    static cairo_surface_t *bgndImage = NULL;
    static cairo_surface_t *menuBgndImage = NULL;

    bool useWindow = (isWindow ||
                      (opts.bgndImage.type == opts.menuBgndImage.type &&
                       (opts.bgndImage.type != IMG_FILE ||
                        (opts.bgndImage.height == opts.menuBgndImage.height &&
                         opts.bgndImage.width == opts.menuBgndImage.width &&
                         opts.bgndImage.pixmap.file ==
                         opts.menuBgndImage.pixmap.file))));
    QtCImage *img = useWindow ? &opts.bgndImage : &opts.menuBgndImage;
    int imgWidth = img->type == IMG_FILE ? img->width : RINGS_WIDTH(img->type);
    int imgHeight = (img->type == IMG_FILE ? img->height :
                     RINGS_HEIGHT(img->type));

    switch (img->type) {
    case IMG_NONE:
        break;
    case IMG_FILE:
        qtcLoadBgndImage(img);
        if (img->pixmap.img) {
            switch (img->pos) {
            case PP_TL:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img, x, y);
                break;
            case PP_TM:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img,
                                            x + (width - img->width) / 2, y);
                break;
            default:
            case PP_TR:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img,
                                            x + width - img->width - 1, y);
                break;
            case PP_BL:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img, x,
                                            y + height - img->height);
                break;
            case PP_BM:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img,
                                            x + (width - img->width) / 2,
                                            y + height - img->height - 1);
                break;
            case PP_BR:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img,
                                            x + width - img->width - 1,
                                            y + height - img->height - 1);
                break;
            case PP_LM:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img, x,
                                            y + (height - img->height) / 2);
                break;
            case PP_RM:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img,
                                            x + width - img->width - 1,
                                            y + (height - img->height) / 2);
                break;
            case PP_CENTRED:
                gdk_cairo_set_source_pixbuf(cr, img->pixmap.img,
                                            x + (width - img->width) / 2,
                                            y + (height - img->height) / 2);
            }
            cairo_paint(cr);
        }
        break;
    case IMG_PLAIN_RINGS:
    case IMG_BORDERED_RINGS: {
        cairo_surface_t *crImg = useWindow ? bgndImage : menuBgndImage;

        if (!crImg) {
            cairo_t *ci;
            crImg = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                               imgWidth + 1, imgHeight + 1);
            ci = cairo_create(crImg);
            drawBgndRing(ci, 0, 0, 200, 140, isWindow);

            drawBgndRing(ci, 210, 10, 230, 214, isWindow);
            drawBgndRing(ci, 226, 26, 198, 182, isWindow);
            drawBgndRing(ci, 300, 100, 50, 0, isWindow);

            drawBgndRing(ci, 100, 96, 160, 144, isWindow);
            drawBgndRing(ci, 116, 112, 128, 112, isWindow);

            drawBgndRing(ci, 250, 160, 200, 140, isWindow);
            drawBgndRing(ci, 310, 220, 80, 0, isWindow);
            cairo_destroy(ci);
            if (useWindow) {
                bgndImage = crImg;
            } else {
                menuBgndImage = crImg;
            }
        }

        cairo_set_source_surface(cr, crImg, width - imgWidth, y + 1);
        cairo_paint(cr);
        break;
    }
    case IMG_SQUARE_RINGS: {
        cairo_surface_t *crImg = useWindow ? bgndImage : menuBgndImage;

        if (!crImg) {
            double halfWidth = RINGS_SQUARE_LINE_WIDTH / 2.0;
            crImg = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                               imgWidth + 1, imgHeight + 1);
            cairo_t *ci = cairo_create(crImg);

            cairo_set_source_rgba(ci, 1.0, 1.0, 1.0, RINGS_SQUARE_SMALL_ALPHA);
            cairo_set_line_width(ci, RINGS_SQUARE_LINE_WIDTH);
            qtcCairoPathWhole(ci, halfWidth + 0.5, halfWidth + 0.5,
                              RINGS_SQUARE_SMALL_SIZE, RINGS_SQUARE_SMALL_SIZE,
                              RINGS_SQUARE_RADIUS, ROUNDED_ALL);
            cairo_stroke(ci);

            cairo_new_path(ci);
            cairo_set_source_rgba(ci, 1.0, 1.0, 1.0, RINGS_SQUARE_SMALL_ALPHA);
            cairo_set_line_width(ci, RINGS_SQUARE_LINE_WIDTH);
            qtcCairoPathWhole(ci, halfWidth + 0.5 + imgWidth -
                              RINGS_SQUARE_SMALL_SIZE - RINGS_SQUARE_LINE_WIDTH,
                              halfWidth + 0.5 + imgHeight -
                              RINGS_SQUARE_SMALL_SIZE - RINGS_SQUARE_LINE_WIDTH,
                              RINGS_SQUARE_SMALL_SIZE, RINGS_SQUARE_SMALL_SIZE,
                              RINGS_SQUARE_RADIUS, ROUNDED_ALL);
            cairo_stroke(ci);

            cairo_new_path(ci);
            cairo_set_source_rgba(ci, 1.0, 1.0, 1.0, RINGS_SQUARE_LARGE_ALPHA);
            cairo_set_line_width(ci, RINGS_SQUARE_LINE_WIDTH);
            qtcCairoPathWhole(ci, halfWidth + 0.5 +
                              (imgWidth - RINGS_SQUARE_LARGE_SIZE -
                               RINGS_SQUARE_LINE_WIDTH) / 2.0,
                              halfWidth + 0.5 +
                              (imgHeight - RINGS_SQUARE_LARGE_SIZE -
                               RINGS_SQUARE_LINE_WIDTH) / 2.0,
                              RINGS_SQUARE_LARGE_SIZE, RINGS_SQUARE_LARGE_SIZE,
                              RINGS_SQUARE_RADIUS, ROUNDED_ALL);
            cairo_stroke(ci);

            cairo_destroy(ci);

            if (useWindow) {
                bgndImage = crImg;
            } else {
                menuBgndImage = crImg;
            }
        }

        cairo_set_source_surface(cr, crImg, width-imgWidth, y+1);
        cairo_paint(cr);
        break;
    }
    }
}

void
drawBgndImage(cairo_t *cr, int x, int y, int w, int h, bool isWindow)
{
    GdkPixbuf *pix = isWindow ? opts.bgndPixmap.img : opts.menuBgndPixmap.img;
    if (pix) {
        gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
        cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_REPEAT);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill(cr);
    }
}

void
drawStripedBgnd(cairo_t *cr, int x, int y, int w, int h,
                const GdkColor *col, double alpha)
{
    GdkColor col2;
    qtcShade(col, &col2, BGND_STRIPE_SHADE, opts.shading);

    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + 4);
    qtcCairoPatternAddColorStop(pat, 0.0, col, alpha);
    qtcCairoPatternAddColorStop(pat, 0.25 - 0.0001, col, alpha);
    qtcCairoPatternAddColorStop(pat, 0.5, &col2, alpha);
    qtcCairoPatternAddColorStop(pat, 0.75 - 0.0001, &col2, alpha);
    col2.red = (3 * col->red + col2.red) / 4;
    col2.green = (3 * col->green + col2.green) / 4;
    col2.blue = (3 * col->blue + col2.blue) / 4;
    qtcCairoPatternAddColorStop(pat, 0.25, &col2, alpha);
    qtcCairoPatternAddColorStop(pat, 0.5 - 0.0001, &col2, alpha);
    qtcCairoPatternAddColorStop(pat, 0.75, &col2, alpha);
    qtcCairoPatternAddColorStop(pat, CAIRO_GRAD_END, &col2, alpha);

    cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
}

bool
drawWindowBgnd(cairo_t *cr, GtkStyle *style, const QtcRect *area,
               GdkWindow *window, GtkWidget *widget, int x, int y,
               int width, int height)
{
    GtkWidget *parent = NULL;
    if (widget && (parent = gtk_widget_get_parent(widget)) &&
        isOnHandlebox(parent, NULL, 0)) {
        return true;
    }
    if (!isFixedWidget(widget) && !isGimpDockable(widget)) {
        int wx = 0;
        int wy = 0;
        int wh = 0;
        int ww = 0;

        if (!mapToTopLevel(window, widget, &wx, &wy, &ww, &wh)) {
            return false;
        }
        GtkWidget *topLevel = gtk_widget_get_toplevel(widget);
        int opacity = (!topLevel || !GTK_IS_DIALOG(topLevel) ?
                       opts.bgndOpacity : opts.dlgOpacity);
        int xmod = 0;
        int ymod = 0;
        int wmod = 0;
        int hmod = 0;
        double alpha = 1.0;
        bool useAlpha = (opacity < 100 && isRgbaWidget(topLevel) &&
                         compositingActive(topLevel));
        bool flatBgnd = qtcIsFlatBgnd(opts.bgndAppearance);

        // Determine the color to use here
        // In commit 87404dba2447c8cba1c70bde72434c67642a7e7c:
        //     When drawing background gradients, use the background colour of
        //     the top-level widget's style.
        // the order was set to Toplevel -> Parent -> Global style.
        // This cause wrong color to be used in chomium.
        // The current order is Parent -> Toplevel -> Global style.
        // Keep this order unless other problems are found.
        // It may also be a good idea to figure out what is the original color
        // and make the color of choice here consistent with that.
        // (at least for chromium)
        const GdkColor *col = getParentBgCol(widget);
        if (!col) {
            col = &qtcDefault(gtk_widget_get_style(topLevel),
                              style)->bg[GTK_STATE_NORMAL];
        }
        if (!flatBgnd || (opts.bgndImage.type == IMG_FILE &&
                          opts.bgndImage.onBorder)) {
            WindowBorders borders = qtcGetWindowBorderSize(false);
            xmod = borders.sides;
            ymod = borders.titleHeight;
            wmod = 2 * borders.sides;
            hmod = borders.titleHeight + borders.bottom;
            wy += ymod;
            wx += xmod;
            wh += hmod;
            ww += wmod;
        }
        QtcRect clip = {x, y, width, height};
        cairo_save(cr);
        qtcCairoClipRect(cr, &clip);
        if (useAlpha) {
            alpha = opacity / 100.0;
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        }
        if (flatBgnd) {
            qtcCairoRect(cr, area, -wx, -wy, ww, wh, col, alpha);
        } else if (opts.bgndAppearance == APPEARANCE_STRIPED) {
            drawStripedBgnd(cr, -wx, -wy, ww, wh, col, alpha);
        } else if (opts.bgndAppearance == APPEARANCE_FILE) {
            cairo_save(cr);
            cairo_translate(cr, -wx, -wy);
            drawBgndImage(cr, 0, 0, ww, wh, true);
            cairo_restore(cr);
        } else {
            drawBevelGradient(cr, area, -wx, -wy, ww, wh + 1, col,
                              opts.bgndGrad == GT_HORIZ, false,
                              opts.bgndAppearance, WIDGET_OTHER, alpha);
            if (opts.bgndGrad == GT_HORIZ &&
                qtcGetGradient(opts.bgndAppearance,
                               &opts)->border == GB_SHINE) {
                int size = qtcMin(BGND_SHINE_SIZE, qtcMin(wh * 2, ww));
                double alpha = qtcShineAlpha(col);
                cairo_pattern_t *pat = NULL;

                size /= BGND_SHINE_STEPS;
                size *= BGND_SHINE_STEPS;
                pat = cairo_pattern_create_radial(
                    ww / 2.0 - wx, -wy, 0, ww / 2.0 - wx, -wy, size / 2.0);
                cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, alpha);
                cairo_pattern_add_color_stop_rgba(pat, 0.5, 1, 1, 1,
                                                  alpha * 0.625);
                cairo_pattern_add_color_stop_rgba(pat, 0.75, 1, 1, 1,
                                                  alpha * 0.175);
                cairo_pattern_add_color_stop_rgba(pat, CAIRO_GRAD_END,
                                                  1, 1, 1, 0.0);
                cairo_set_source(cr, pat);
                cairo_rectangle(cr, (ww - size) / 2.0 - wx, -wy, size, size);
                cairo_fill(cr);
                cairo_pattern_destroy(pat);
            }
        }
        if (useAlpha) {
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        }
        if (!(opts.bgndImage.type == IMG_FILE && opts.bgndImage.onBorder)) {
            ww -= wmod + 1;
            wh -= hmod;
            wx -= xmod;
            wy -= ymod;
        }
        drawBgndRings(cr, -wx, -wy, ww, wh, true);
        cairo_restore(cr);
        return true;
    }
    return false;
}

void
drawEntryField(cairo_t *cr, GtkStyle *style, GtkStateType state,
               GdkWindow *window, GtkWidget *widget, const QtcRect *area,
               int x, int y, int width, int height,
               ECornerBits round, EWidget w)
{
    bool enabled = !(GTK_STATE_INSENSITIVE == state ||
                     (widget && !gtk_widget_is_sensitive(widget)));
    bool highlightReal =
        (enabled && widget && gtk_widget_has_focus(widget) &&
         qtSettings.app != GTK_APP_JAVA && qtcPalette.focus);
    bool mouseOver =
        (opts.unifyCombo && opts.unifySpin && enabled &&
         (GTK_STATE_PRELIGHT == state || qtcEntryIsLastMo(widget)) &&
         qtcPalette.mouseover && GTK_APP_JAVA != qtSettings.app);
    bool highlight = highlightReal || mouseOver;
    bool doEtch = opts.buttonEffect != EFFECT_NONE && opts.etchEntry;
    bool comboOrSpin = qtcOneOf(w, WIDGET_SPIN, WIDGET_COMBO_BUTTON);
    const GdkColor *colors = (mouseOver ? qtcPalette.mouseover : highlightReal ?
                              qtcPalette.focus : qtcPalette.background);

    if (qtSettings.app != GTK_APP_JAVA) {
        qtcEntrySetup(widget);
    }
    if ((doEtch || opts.round != ROUND_NONE) &&
        (!widget || !g_object_get_data(G_OBJECT(widget),
                                       "transparent-bg-hint"))) {
        if (qtcIsFlatBgnd(opts.bgndAppearance) || !widget ||
            !drawWindowBgnd(cr, style, area, window, widget, x, y,
                            width, height)) {
            GdkColor parentBgCol;
            getEntryParentBgCol(widget, &parentBgCol);
            drawEntryCorners(cr, area, round, x, y, width, height,
                             &parentBgCol, 1.0);
        }
    }

    if (opts.gbFactor != 0 &&
        qtcOneOf(opts.groupBox, FRAME_SHADED, FRAME_FADED) &&
        isInGroupBox(widget, 0)) {
        GdkColor col;
        col.red = col.green = col.blue = opts.gbFactor < 0 ? 0.0 : 1.0;
        drawEntryCorners(cr, area, round, x, y, width, height, &col,
                         TO_ALPHA(opts.gbFactor));
    }

    if (doEtch) {
        y++;
        x++;
        height -= 2;
        width -= 2;
    }

    if (DEBUG_ALL == qtSettings.debug) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %d ", __FUNCTION__,
               state, x, y, width, height, round);
        debugDisplayWidget(widget, 10);
    }

    if (round != ROUNDED_ALL) {
        if (comboOrSpin) {
            x -= 2;
            width += 2;
        } else if (highlight) {
            if (doEtch) {
                if (round == ROUNDED_RIGHT) {
                    /* RtoL */
                    x--;
                } else {
                    width++;
                }
            }
        } else {
            if (round == ROUNDED_RIGHT) {
                /* RtoL */
                x -= 2;
            } else {
                width += 2;
            }
        }
    }

    cairo_save(cr);
    if (opts.round > ROUND_FULL) {
        qtcClipPath(cr, x + 1, y + 1, width - 2, height - 2,
                    WIDGET_ENTRY, RADIUS_INTERNAL, ROUNDED_ALL);
    }
    qtcCairoRect(cr, area, x + 1, y + 1, width - 2, height - 2,
                 enabled ? &style->base[GTK_STATE_NORMAL] :
                 &style->bg[GTK_STATE_INSENSITIVE]);
    cairo_restore(cr);

    cairo_save(cr);
    if (qtSettings.app == GTK_APP_OPEN_OFFICE && comboOrSpin) {
        const QtcRect rect = {x, y, width, height};
        x -= 4;
        width += 4;
        qtcCairoClipRect(cr, &rect);
    }

    int xo = x;
    int yo = y;
    int widtho = width;
    int heighto = height;

    if (doEtch) {
        y--;
        height += 2;
        x--;
        width += 2;
        if (!(w == WIDGET_SPIN && opts.unifySpin) &&
            !(w == WIDGET_COMBO_BUTTON && opts.unifyCombo)) {
            if (!(round & CORNER_TR) && !(round & CORNER_BR)) {
                width += 4;
            }
            if (!(round & CORNER_TL) && !(round & CORNER_BL)) {
                x -= 4;
            }
        }
        drawEtch(cr, area, widget, x, y, width, height, false, round,
                 WIDGET_ENTRY);
    }

    drawBorder(cr, style, (!widget || gtk_widget_is_sensitive(widget) ?
                           state : GTK_STATE_INSENSITIVE), area,
               xo, yo, widtho, heighto, colors, round,
               BORDER_SUNKEN, WIDGET_ENTRY, DF_BLEND);
    cairo_restore(cr);
    if (GTK_IS_ENTRY(widget) && !gtk_entry_get_visibility(GTK_ENTRY(widget))) {
        gtk_entry_set_invisible_char(GTK_ENTRY(widget), opts.passwordChar);
    }
}

static void
qtcSetProgressStripeClipping(cairo_t *cr, const QtcRect *area, int x, int y,
                             int width, int height, int animShift, bool horiz)
{
    switch (opts.stripedProgress) {
    default:
    case STRIPE_PLAIN: {
        QtcRect rect = {x, y, width - 2, height - 2};
        qtcRectConstrain(&rect, area);
        cairo_region_t *region = cairo_region_create_rectangle(&rect);
        if (horiz) {
            for (int stripeOffset = 0;
                 stripeOffset < width + PROGRESS_CHUNK_WIDTH;
                 stripeOffset += PROGRESS_CHUNK_WIDTH * 2) {
                QtcRect innerRect = {x + stripeOffset + animShift, y + 1,
                                     PROGRESS_CHUNK_WIDTH, height - 2};
                qtcRectConstrain(&innerRect, area);
                if (innerRect.width > 0 && innerRect.height > 0) {
                    cairo_region_xor_rectangle(region, &innerRect);
                }
            }
        } else {
            for (int stripeOffset = 0;
                 stripeOffset < height + PROGRESS_CHUNK_WIDTH;
                 stripeOffset += PROGRESS_CHUNK_WIDTH * 2) {
                QtcRect innerRect = {x + 1, y + stripeOffset + animShift,
                                     width - 2, PROGRESS_CHUNK_WIDTH};
                /* qtcRectConstrain(&innerRect, area); */
                if (innerRect.width > 0 && innerRect.height > 0) {
                    cairo_region_xor_rectangle(region, &innerRect);
                }
            }
        }
        qtcCairoClipRegion(cr, region);
        cairo_region_destroy(region);
        break;
    }
    case STRIPE_DIAGONAL:
        cairo_new_path(cr);
        /* if (area) */
        /*     cairo_rectangle(cr, area->x, area->y, */
        /*                     area->width, area->height); */
        if (horiz) {
            for (int stripeOffset = 0;stripeOffset < width + height + 2;
                 stripeOffset += PROGRESS_CHUNK_WIDTH * 2) {
                const GdkPoint pts[4] = {
                    {x + stripeOffset + animShift, y},
                    {x + stripeOffset + animShift + PROGRESS_CHUNK_WIDTH, y},
                    {x + stripeOffset + animShift +
                     PROGRESS_CHUNK_WIDTH - height, y + height - 1},
                    {x + stripeOffset + animShift - height, y + height - 1}};
                qtcCairoPathPoints(cr, pts, 4);
            }
        } else {
            for (int stripeOffset = 0;stripeOffset < height + width + 2;
                 stripeOffset += PROGRESS_CHUNK_WIDTH * 2) {
                const GdkPoint pts[4] = {
                    {x, y + stripeOffset + animShift},
                    {x + width - 1, y + stripeOffset + animShift - width},
                    {x + width - 1, y + stripeOffset + animShift +
                     PROGRESS_CHUNK_WIDTH - width},
                    {x, y + stripeOffset + animShift + PROGRESS_CHUNK_WIDTH}};
                qtcCairoPathPoints(cr, pts, 4);
            }
        }
        cairo_clip(cr);
    }
}

void
drawProgress(cairo_t *cr, GtkStyle *style, GtkStateType state,
             GtkWidget *widget, const QtcRect *area, int x, int y,
             int width, int height, bool rev, bool isEntryProg)
{
#if GTK_CHECK_VERSION(2, 90, 0)
    bool revProg = (widget && GTK_IS_PROGRESS_BAR(widget) &&
                    gtk_progress_bar_get_inverted(GTK_PROGRESS_BAR(widget)));
#else
    GtkProgressBarOrientation orientation =
        (widget && GTK_IS_PROGRESS_BAR(widget) ?
         gtk_progress_bar_get_orientation(GTK_PROGRESS_BAR(widget)) :
         GTK_PROGRESS_LEFT_TO_RIGHT);
    bool revProg = qtcOneOf(orientation, GTK_PROGRESS_RIGHT_TO_LEFT,
                            GTK_PROGRESS_BOTTOM_TO_TOP);
#endif
    const bool horiz = isHorizontalProgressbar(widget);
    EWidget wid = isEntryProg ? WIDGET_ENTRY_PROGRESSBAR : WIDGET_PROGRESSBAR;
    int animShift = revProg ? 0 : -PROGRESS_CHUNK_WIDTH;
    int xo = x;
    int yo = y;
    int wo = width;
    int ho = height;

    if (opts.fillProgress) {
        x--;
        y--;
        width += 2;
        height += 2;
        xo = x;
        yo = y;
        wo = width;
        ho = height;
    }

    if (opts.stripedProgress != STRIPE_NONE &&
        opts.animatedProgress && (isEntryProg || qtcIsProgressBar(widget))) {
#if !GTK_CHECK_VERSION(2, 90, 0) /* Gtk3:TODO !!! */
        if (isEntryProg || !GTK_PROGRESS(widget)->activity_mode)
#endif
            QtCurve::Animation::addProgressBar(widget, isEntryProg);

        animShift+=(revProg ? -1 : 1)*
            (((int)(QtCurve::Animation::elapsed(widget) *
                    PROGRESS_CHUNK_WIDTH)) %
             (PROGRESS_CHUNK_WIDTH * 2));
    }

    bool grayItem = (state == GTK_STATE_INSENSITIVE &&
                     opts.progressGrooveColor != ECOLOR_BACKGROUND);
    const GdkColor *itemCols = (grayItem ? qtcPalette.background :
                                qtcPalette.progress ? qtcPalette.progress :
                                qtcPalette.highlight);
    auto new_state = GTK_STATE_PRELIGHT == state ? GTK_STATE_NORMAL : state;
    int fillVal = grayItem ? 4 : ORIGINAL_SHADE;

    x++;
    y++;
    width -= 2;
    height -= 2;

    cairo_save(cr);
    if (opts.borderProgress && opts.round > ROUND_SLIGHT &&
        (horiz ? width : height) < 4) {
        qtcClipPath(cr, x, y, width, height, wid, RADIUS_EXTERNAL, ROUNDED_ALL);
    }

    if ((horiz ? width : height) > 1)
        drawLightBevel(cr, style, new_state, area, x, y, width,
                       height, &itemCols[fillVal], itemCols, ROUNDED_ALL,
                       wid, BORDER_FLAT, (horiz ? 0 : DF_VERT), widget);
    if (opts.stripedProgress && width > 4 && height > 4) {
        if (opts.stripedProgress == STRIPE_FADE) {
            int posMod = opts.animatedProgress ? STRIPE_WIDTH - animShift : 0;
            int sizeMod = opts.animatedProgress ? (STRIPE_WIDTH * 2) : 0;
            addStripes(cr, x - (horiz ? posMod : 0), y - (horiz ? 0 : posMod),
                       width + (horiz ? sizeMod : 0),
                       height + (horiz ? 0 : sizeMod), horiz);
        } else {
            cairo_save(cr);
            qtcSetProgressStripeClipping(cr, area, xo, yo, wo, ho,
                                         animShift, horiz);
            drawLightBevel(cr, style, new_state, NULL, x, y,
                           width, height, &itemCols[1], qtcPalette.highlight,
                           ROUNDED_ALL, wid, BORDER_FLAT,
                           (opts.fillProgress || !opts.borderProgress ? 0 :
                            DF_DO_BORDER) | (horiz ? 0 : DF_VERT), widget);
            cairo_restore(cr);
        }
    }
    if (opts.glowProgress && (horiz ? width : height)>3) {
        int offset = opts.borderProgress ? 1 : 0;
        cairo_pattern_t *pat =
            cairo_pattern_create_linear(x+offset, y+offset, horiz ?
                                        x + width - offset : x + offset,
                                        horiz ? y + offset :
                                        y + height - offset);
        bool inverted = false;

        if (GLOW_MIDDLE != opts.glowProgress && widget &&
            GTK_IS_PROGRESS_BAR(widget)) {
            if (horiz) {
                if (revProg) {
                    inverted = !rev;
                } else {
                    inverted = rev;
                }
            } else {
                inverted = revProg;
            }
        }

        cairo_pattern_add_color_stop_rgba(
            pat, 0.0, 1.0, 1.0, 1.0,
            (inverted ? GLOW_END : GLOW_START) == opts.glowProgress ?
            GLOW_PROG_ALPHA : 0.0);

        if (GLOW_MIDDLE == opts.glowProgress)
            cairo_pattern_add_color_stop_rgba(pat, 0.5, 1.0, 1.0, 1.0,
                                              GLOW_PROG_ALPHA);

        cairo_pattern_add_color_stop_rgba(
            pat, CAIRO_GRAD_END, 1.0, 1.0, 1.0,
            (inverted ? GLOW_START : GLOW_END) == opts.glowProgress ?
            GLOW_PROG_ALPHA : 0.0);
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, x + offset, y + offset, width - 2 * offset,
                        height - 2 * offset);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        if (width > 2 && height > 2 && opts.borderProgress)
            drawBorder(cr, style, state, area, x, y, width, height,
                       itemCols, ROUNDED_ALL, BORDER_FLAT, wid,
                       0, PBAR_BORDER);

        if (!opts.borderProgress) {
            if (horiz) {
                qtcCairoHLine(cr, x, y, width, &itemCols[PBAR_BORDER]);
                qtcCairoHLine(cr, x, y + height - 1, width,
                              &itemCols[PBAR_BORDER]);
            } else {
                qtcCairoVLine(cr, x, y, height, &itemCols[PBAR_BORDER]);
                qtcCairoVLine(cr, x + width - 1, y, height,
                              &itemCols[PBAR_BORDER]);
            }
        }
    }
    cairo_restore(cr);
}

void
drawProgressGroove(cairo_t *cr, GtkStyle *style, GtkStateType state,
                   GdkWindow *window, GtkWidget *widget, const QtcRect *area,
                   int x, int y, int width, int height, bool isList, bool horiz)
{
    bool doEtch = !isList && opts.buttonEffect != EFFECT_NONE;
    const GdkColor *col = &style->base[state];
    int offset = opts.borderProgress ? 1 : 0;

    switch (opts.progressGrooveColor) {
    default:
    case ECOLOR_BASE:
        col = &style->base[state];
        break;
    case ECOLOR_BACKGROUND:
        col = &qtcPalette.background[ORIGINAL_SHADE];
        break;
    case ECOLOR_DARK:
        col = &qtcPalette.background[2];
    }

    if (!isList && (qtcIsFlatBgnd(opts.bgndAppearance) ||
                    !(widget && drawWindowBgnd(cr, style, area, window, widget,
                                               x, y, width, height))) &&
        (!widget || !g_object_get_data(G_OBJECT(widget),
                                       "transparent-bg-hint"))) {
        qtcCairoRect(cr, area, x, y, width, height,
            &qtcPalette.background[ORIGINAL_SHADE]);
    }

    if (doEtch && opts.borderProgress) {
        x++;
        y++;
        width -= 2;
        height -= 2;
    }

    drawBevelGradient(cr, area, x + offset, y + offset,
                      width - 2 * offset, height - 2 * offset, col, horiz,
                      false, opts.progressGrooveAppearance, WIDGET_PBAR_TROUGH);

    if (doEtch && opts.borderProgress) {
        drawEtch(cr, area, widget, x - 1, y - 1, width + 2,
                 height + 2, false, ROUNDED_ALL, WIDGET_PBAR_TROUGH);
    }
    if (opts.borderProgress) {
        GtkWidget *parent = widget ? gtk_widget_get_parent(widget) : NULL;
        GtkStyle *style = widget ? gtk_widget_get_style(parent ?
                                                        parent : widget) : NULL;
        drawBorder(cr, style, state, area, x, y, width, height,
                   NULL, ROUNDED_ALL,
                   (qtcIsFlat(opts.progressGrooveAppearance) &&
                    opts.progressGrooveColor != ECOLOR_DARK ?
                    BORDER_SUNKEN : BORDER_FLAT),
                   WIDGET_PBAR_TROUGH, DF_BLEND);
    } else { /* if(!opts.borderProgress) */
        if (horiz) {
            qtcCairoHLine(cr, x, y, width,
                          &qtcPalette.background[QTC_STD_BORDER]);
            qtcCairoHLine(cr, x, y + height - 1, width,
                          &qtcPalette.background[QTC_STD_BORDER]);
        } else {
            qtcCairoVLine(cr, x, y, height,
                          &qtcPalette.background[QTC_STD_BORDER]);
            qtcCairoVLine(cr, x + width - 1, y, height,
                          &qtcPalette.background[QTC_STD_BORDER]);
        }
    }
}

#define SLIDER_TROUGH_SIZE 5

void
drawSliderGroove(cairo_t *cr, GtkStyle *style, GtkStateType state,
                 GtkWidget *widget, const char *detail,
                 const QtcRect *area, int x, int y, int width, int height,
                 bool horiz)
{
    const GdkColor *bgndcols = qtcPalette.background;
    const GdkColor *bgndcol = &bgndcols[2];
    GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(widget));
    double upper = gtk_adjustment_get_upper(adjustment);
    double lower = gtk_adjustment_get_lower(adjustment);
    double value = gtk_adjustment_get_value(adjustment);
    int used_x = x;
    int used_y = y;
    int used_h = 0;
    int used_w = 0;
    int pos = (int)(((double)(horiz ? width : height) /
                     (upper - lower))  * (value - lower));
    bool inverted = gtk_range_get_inverted(GTK_RANGE(widget));
    bool doEtch = opts.buttonEffect != EFFECT_NONE;
    bool rev = (reverseLayout(widget) ||
                (widget && reverseLayout(gtk_widget_get_parent(widget))));
    int troughSize = SLIDER_TROUGH_SIZE + (doEtch ? 2 : 0);
    const GdkColor *usedcols =
        (opts.fillSlider && upper != lower && state != GTK_STATE_INSENSITIVE ?
         qtcPalette.slider ? qtcPalette.slider : qtcPalette.highlight :
         qtcPalette.background);
    EWidget wid = WIDGET_SLIDER_TROUGH;

    if (horiz && rev)
        inverted = !inverted;

    if (horiz) {
        y += (height - troughSize) / 2;
        height = troughSize;
        used_y = y;
        used_h = height;
    } else {
        x += (width - troughSize) / 2;
        width = troughSize;
        used_x = x;
        used_w = width;
    }

    if (state == GTK_STATE_INSENSITIVE) {
        bgndcol = &bgndcols[ORIGINAL_SHADE];
    } else if (strcmp(detail, "trough-lower") == 0 && opts.fillSlider) {
        bgndcols = usedcols;
        bgndcol = &usedcols[ORIGINAL_SHADE];
        wid = WIDGET_FILLED_SLIDER_TROUGH;
    }
    drawLightBevel(cr, style, state, area, x, y, width, height, bgndcol,
                   bgndcols, (opts.square & SQUARE_SLIDER ? ROUNDED_NONE :
                              ROUNDED_ALL), wid, BORDER_FLAT,
                   DF_SUNKEN | DF_DO_BORDER | (horiz ? 0 : DF_VERT), NULL);

    if (opts.fillSlider && upper != lower && state != GTK_STATE_INSENSITIVE &&
        strcmp(detail, "trough") == 0) {
        if (horiz) {
            pos += width > 10 && pos < width / 2 ? 3 : 0;

            if (inverted) {
                /* <rest><slider><used> */
                used_x += width - pos;
            }
            used_w = pos;
        } else {
            pos += height > 10 && pos < height / 2 ? 3 : 0;

            if (inverted) {
                used_y += height - pos;
            }
            used_h = pos;
        }

        if (used_w > 0 && used_h > 0) {
            drawLightBevel(cr, style, state, area, used_x, used_y, used_w,
                           used_h, &usedcols[ORIGINAL_SHADE], usedcols,
                           opts.square & SQUARE_SLIDER ? ROUNDED_NONE :
                           ROUNDED_ALL, WIDGET_FILLED_SLIDER_TROUGH,
                           BORDER_FLAT, DF_SUNKEN | DF_DO_BORDER |
                           (horiz ? 0 : DF_VERT), NULL);
        }
    }
}

void
drawTriangularSlider(cairo_t *cr, GtkStyle *style, GtkStateType state,
                     const char *detail, int x, int y, int width, int height)
{
    GdkColor newColors[TOTAL_SHADES + 1];
    const GdkColor *btnColors = NULL;

    /* Fix Java swing sliders looking pressed */
    if (state == GTK_STATE_ACTIVE)
        state = GTK_STATE_PRELIGHT;

    if (useButtonColor(detail)) {
        if (state == GTK_STATE_INSENSITIVE) {
            btnColors = qtcPalette.background;
        } else if (QT_CUSTOM_COLOR_BUTTON(style)) {
            qtcShadeColors(&style->bg[state], newColors);
            btnColors = newColors;
        } else {
            GtkWidget *widget = NULL; /* Keep SET_BTN_COLS happy */
            SET_BTN_COLS(false, true, false, state);
        }
    }

    bool coloredMouseOver = (state == GTK_STATE_PRELIGHT &&
                             opts.coloredMouseOver &&
                             !opts.colorSliderMouseOver);
    bool horiz = height > width || DETAIL("hscale");
    int bgnd = getFill(state, false, opts.shadeSliders == SHADE_DARKEN);
    int xo = horiz ? 8 : 0;
    int yo = horiz ? 0 : 8;
    int size = 15;
    int light = opts.sliderAppearance == APPEARANCE_DULL_GLASS ? 1 : 0;
    const GdkColor *colors = btnColors;
    const GdkColor *borderCols = ((GTK_STATE_PRELIGHT == state &&
                                   qtcOneOf(opts.coloredMouseOver, MO_GLOW,
                                            MO_COLORED)) ?
                                  qtcPalette.mouseover : btnColors);
    GtkArrowType direction = horiz ? GTK_ARROW_DOWN : GTK_ARROW_RIGHT;
    bool drawLight = opts.coloredMouseOver != MO_PLASTIK || !coloredMouseOver;
    int borderVal = (qtcPalette.mouseover == borderCols ? SLIDER_MO_BORDER_VAL :
                     BORDER_VAL(state == GTK_STATE_INSENSITIVE));
    if (opts.coloredMouseOver == MO_GLOW && opts.buttonEffect != EFFECT_NONE) {
        x++;
        y++;
        xo++;
        yo++;
    }
    cairo_new_path(cr);
    cairo_save(cr);
    switch (direction) {
    case GTK_ARROW_UP:
    default:
    case GTK_ARROW_DOWN: {
        y += 2;
        const GdkPoint pts[] = {{x, y}, {x, y + 2}, {x + 2, y}, {x + 8, y},
                                {x + 10, y + 2}, {x + 10, y + 9},
                                {x + 5, y + 14}, {x, y + 9}};
        qtcCairoPathPoints(cr, pts, 8);
        break;
    }
    case GTK_ARROW_RIGHT:
    case GTK_ARROW_LEFT: {
        x+=2;
        const GdkPoint pts[] = {{x, y}, {x + 2, y}, {x, y + 2}, {x, y + 8},
                                {x + 2, y + 10}, {x + 9, y + 10},
                                {x + 14, y + 5}, {x + 9, y}};
        qtcCairoPathPoints(cr, pts, 8);
    }
    }
    cairo_clip(cr);
    if (qtcIsFlat(opts.sliderAppearance)) {
        qtcCairoRect(cr, NULL, x + 1, y + 1, width - 2, height - 2,
                     &colors[bgnd]);
        if (MO_PLASTIK == opts.coloredMouseOver && coloredMouseOver) {
            int col = SLIDER_MO_SHADE;
            int len = SLIDER_MO_LEN;
            if (horiz) {
                qtcCairoRect(cr, NULL, x + 1, y + 1, len, size - 2,
                             &qtcPalette.mouseover[col]);
                qtcCairoRect(cr, NULL, x + width - (1 + len), y + 1, len,
                             size - 2, &qtcPalette.mouseover[col]);
            } else {
                qtcCairoRect(cr, NULL, x + 1, y + 1, size - 2, len,
                             &qtcPalette.mouseover[col]);
                qtcCairoRect(cr, NULL, x + 1, y + height - (1 + len), size - 2,
                             len, &qtcPalette.mouseover[col]);
            }
        }
    } else {
        drawBevelGradient(cr, NULL, x, y, horiz ? width - 1 : size,
                          horiz ? size : height - 1, &colors[bgnd],
                          horiz, false, MODIFY_AGUA(opts.sliderAppearance),
                          WIDGET_OTHER);
        if (opts.coloredMouseOver == MO_PLASTIK && coloredMouseOver) {
            int col = SLIDER_MO_SHADE;
            int len = SLIDER_MO_LEN;
            if (horiz) {
                drawBevelGradient(cr, NULL, x + 1, y + 1, len, size - 2,
                                  &qtcPalette.mouseover[col], horiz, false,
                                  MODIFY_AGUA(opts.sliderAppearance),
                                  WIDGET_OTHER);
                drawBevelGradient(cr, NULL, x + width - (1 + len), y + 1, len,
                                  size - 2, &qtcPalette.mouseover[col], horiz,
                                  false, MODIFY_AGUA(opts.sliderAppearance),
                                  WIDGET_OTHER);
            } else {
                drawBevelGradient(cr, NULL, x + 1, y + 1, size - 2, len,
                                  &qtcPalette.mouseover[col], horiz, false,
                                  MODIFY_AGUA(opts.sliderAppearance),
                                  WIDGET_OTHER);
                drawBevelGradient(cr, NULL, x + 1, y + height - (1 + len),
                                  size - 2, len, &qtcPalette.mouseover[col],
                                  horiz, false,
                                  MODIFY_AGUA(opts.sliderAppearance),
                                  WIDGET_OTHER);
            }
        }
    }
    cairo_restore(cr);
    double xd = x + 0.5;
    double yd = y + 0.5;
    double radius = 2.5;
    double xdg = xd - 1;
    double ydg = yd - 1;
    double radiusg = radius + 1;
    bool glowMo = (opts.coloredMouseOver == MO_GLOW &&
                   coloredMouseOver && opts.buttonEffect != EFFECT_NONE);
    cairo_new_path(cr);
    if (glowMo) {
        qtcCairoSetColor(cr, &borderCols[GLOW_MO], GLOW_ALPHA(false));
    } else {
        qtcCairoSetColor(cr, &borderCols[borderVal]);
    }
    switch (direction) {
    case GTK_ARROW_UP:
    default:
    case GTK_ARROW_DOWN:
        if (glowMo) {
            cairo_move_to(cr, xdg + radiusg, ydg);
            cairo_arc(cr, xdg + 12 - radiusg, ydg + radiusg, radiusg,
                      M_PI * 1.5, M_PI * 2);
            cairo_line_to(cr, xdg + 12, ydg + 10.5);
            cairo_line_to(cr, xdg + 6, ydg + 16.5);
            cairo_line_to(cr, xdg, ydg + 10.5);
            cairo_arc(cr, xdg + radiusg, ydg + radiusg, radiusg,
                      M_PI, M_PI * 1.5);
            cairo_stroke(cr);
            qtcCairoSetColor(cr, &borderCols[borderVal]);
        }
        cairo_move_to(cr, xd + radius, yd);
        cairo_arc(cr, xd + 10 - radius, yd + radius, radius,
                  M_PI * 1.5, M_PI * 2);
        cairo_line_to(cr, xd + 10, yd + 9);
        cairo_line_to(cr, xd + 5, yd + 14);
        cairo_line_to(cr, xd, yd + 9);
        cairo_arc(cr, xd + radius, yd + radius, radius, M_PI, M_PI * 1.5);
        cairo_stroke(cr);
        if (drawLight) {
            qtcCairoVLine(cr, xd + 1, yd + 2, 7, &colors[light]);
            qtcCairoHLine(cr, xd + 2, yd + 1, 6, &colors[light]);
        }
        break;
    case GTK_ARROW_RIGHT:
    case GTK_ARROW_LEFT:
        if (glowMo) {
            cairo_move_to(cr, xdg, ydg + 12-radiusg);
            cairo_arc(cr, xdg + radiusg, ydg + radiusg,
                      radiusg, M_PI, M_PI * 1.5);
            cairo_line_to(cr, xdg + 10.5, ydg);
            cairo_line_to(cr, xdg + 16.5, ydg + 6);
            cairo_line_to(cr, xdg + 10.5, ydg + 12);
            cairo_arc(cr, xdg + radiusg, ydg + 12 - radiusg,
                      radiusg, M_PI * 0.5, M_PI);
            cairo_stroke(cr);
            qtcCairoSetColor(cr, &borderCols[borderVal]);
        }
        cairo_move_to(cr, xd, yd + 10 - radius);
        cairo_arc(cr, xd + radius, yd + radius, radius, M_PI, M_PI * 1.5);
        cairo_line_to(cr, xd + 9, yd);
        cairo_line_to(cr, xd + 14, yd + 5);
        cairo_line_to(cr, xd + 9, yd + 10);
        cairo_arc(cr, xd + radius, yd + 10 - radius, radius, M_PI * 0.5, M_PI);
        cairo_stroke(cr);
        if (drawLight) {
            qtcCairoHLine(cr, xd + 2, yd + 1, 7, &colors[light]);
            qtcCairoVLine(cr, xd + 1, yd + 2, 6, &colors[light]);
        }
    }
}

void
drawScrollbarGroove(cairo_t *cr, GtkStyle *style, GtkStateType state,
                    GtkWidget *widget, const QtcRect *area, int x, int y,
                    int width, int height, bool horiz)
{
    ECornerBits sbarRound = ROUNDED_ALL;
    int xo = x;
    int yo = y;
    int wo = width;
    int ho = height;
    bool drawBg = opts.flatSbarButtons;
    bool thinner = (opts.thinSbarGroove &&
                    (opts.scrollbarType == SCROLLBAR_NONE ||
                     opts.flatSbarButtons));

    if (opts.flatSbarButtons) {
#if GTK_CHECK_VERSION(2, 90, 0)
        bool lower = detail && strstr(detail, "-lower");
        sbarRound = (lower ? horiz ? ROUNDED_LEFT : ROUNDED_TOP : horiz ?
                     ROUNDED_RIGHT : ROUNDED_BOTTOM);

        switch (opts.scrollbarType) {
        case SCROLLBAR_KDE:
            if (lower) {
                if (horiz) {
                    x += opts.sliderWidth;
                } else {
                    y += opts.sliderWidth;
                }
            } else {
                if (horiz) {
                    width -= opts.sliderWidth * 2;
                } else {
                    height -= opts.sliderWidth * 2;
                }
            }
            break;
        case SCROLLBAR_WINDOWS:
            if (lower) {
                if (horiz) {
                    x += opts.sliderWidth;
                } else {
                    y += opts.sliderWidth;
                }
            } else {
                if (horiz) {
                    width -= opts.sliderWidth;
                } else {
                    height -= opts.sliderWidth;
                }
            }
            break;
        case SCROLLBAR_NEXT:
            if (lower) {
                if (horiz) {
                    x += opts.sliderWidth * 2;
                } else {
                    y += opts.sliderWidth * 2;
                }
            }
            break;
        case SCROLLBAR_PLATINUM:
            if (!lower) {
                if (horiz) {
                    width -= opts.sliderWidth * 2;
                } else {
                    height -= opts.sliderWidth * 2;
                }
            }
            break;
        }
#else
        switch (opts.scrollbarType) {
        case SCROLLBAR_KDE:
            if (horiz) {
                x += opts.sliderWidth;
                width -= opts.sliderWidth * 3;
            } else {
                y += opts.sliderWidth;
                height -= opts.sliderWidth * 3;
            }
            break;
        case SCROLLBAR_WINDOWS:
            if (horiz) {
                x += opts.sliderWidth;
                width -= opts.sliderWidth * 2;
            } else {
                y += opts.sliderWidth;
                height -= opts.sliderWidth * 2;
            }
            break;
        case SCROLLBAR_NEXT:
            if (horiz) {
                x+=opts.sliderWidth * 2;
                width -= opts.sliderWidth * 2;
            } else {
                y += opts.sliderWidth * 2;
                height -= opts.sliderWidth * 2;
            }
            break;
        case SCROLLBAR_PLATINUM:
            if (horiz) {
                width -= opts.sliderWidth * 2;
            } else {
                height -= opts.sliderWidth * 2;
            }
            break;
        default:
            break;
        }
#endif
    } else {
        switch (opts.scrollbarType) {
        default:
            break;
        case SCROLLBAR_NEXT:
            sbarRound = horiz ? ROUNDED_LEFT : ROUNDED_TOP;
            break;
        case SCROLLBAR_PLATINUM:
            sbarRound = horiz ? ROUNDED_RIGHT : ROUNDED_BOTTOM;
            break;
#ifdef SIMPLE_SCROLLBARS
        case SCROLLBAR_NONE:
            sbarRound = ROUNDED_NONE;
            break;
#endif
        }
    }

    if (opts.square & SQUARE_SB_SLIDER)
        sbarRound = ROUNDED_NONE;

    if (drawBg) {
        GtkWidget *parent=NULL;
        if (opts.gtkScrollViews && qtcIsFlat(opts.sbarBgndAppearance) &&
            opts.tabBgnd != 0 && widget &&
           (parent = gtk_widget_get_parent(widget)) &&
            GTK_IS_SCROLLED_WINDOW(parent) &&
            (parent = gtk_widget_get_parent(parent)) &&
            GTK_IS_NOTEBOOK(parent)) {
            drawAreaModColor(cr, area, &qtcPalette.background[ORIGINAL_SHADE],
                             TO_FACTOR(opts.tabBgnd), xo, yo, wo, ho);
        } else if (!qtcIsFlat(opts.sbarBgndAppearance) ||
                   !opts.gtkScrollViews) {
            drawBevelGradient(cr, area, xo, yo, wo, ho,
                              &qtcPalette.background[ORIGINAL_SHADE], horiz,
                              false, opts.sbarBgndAppearance, WIDGET_SB_BGND);
        }
#if !GTK_CHECK_VERSION(2, 90, 0)
        /* This was the old (pre 1.7.1) else if... but it messes up Gtk3 scrollbars wheb have custom background. And dont think its needed */
        /* But re-added in 1.7.2 for Mozilla! */
        /*  else if(!qtcIsCustomBgnd(&opts) || !(opts.gtkScrollViews && qtcIsFlat(opts.sbarBgndAppearance) &&
            widget && drawWindowBgnd(cr, style, area, widget, xo, yo, wo, ho)))
        */ else if (isMozilla()) {
            /* 1.7.3 added 'else' so as to not duplicate above! */
            drawBevelGradient(cr, area, xo, yo, wo, ho,
                              &qtcPalette.background[ORIGINAL_SHADE], horiz,
                              false, opts.sbarBgndAppearance, WIDGET_SB_BGND);
        }
#endif
    }

    if (isMozilla()) {
        if (!drawBg) {
            const GdkColor *parent_col = getParentBgCol(widget);
            const GdkColor *bgnd_col =
                (parent_col ? parent_col :
                 &qtcPalette.background[ORIGINAL_SHADE]);

            cairo_save(cr);
            qtcCairoClipRect(cr, area);
            qtcCairoRect(cr, area, x, y, width, height,
                         &qtcPalette.background[ORIGINAL_SHADE]);
            if (horiz) {
                if (qtcOneOf(sbarRound, ROUNDED_LEFT, ROUNDED_ALL)) {
                    qtcCairoVLine(cr, x, y, height, bgnd_col);
                }
                if (qtcOneOf(sbarRound, ROUNDED_RIGHT, ROUNDED_ALL)) {
                    qtcCairoVLine(cr, x + width - 1, y, height, bgnd_col);
                }
            } else {
                if (qtcOneOf(sbarRound, ROUNDED_TOP, ROUNDED_ALL)) {
                    qtcCairoHLine(cr, x, y, width, bgnd_col);
                }
                if (qtcOneOf(sbarRound, ROUNDED_BOTTOM, ROUNDED_ALL)) {
                    qtcCairoHLine(cr, x, y + height - 1, width, bgnd_col);
                }
            }
            cairo_restore(cr);
        }
    } else if (qtSettings.app == GTK_APP_OPEN_OFFICE &&
               opts.flatSbarButtons && isFixedWidget(widget)) {
        if (horiz) {
            width--;
        } else {
            height--;
        }
    }

    if (thinner && !drawBg)
        drawBgnd(cr, &qtcPalette.background[ORIGINAL_SHADE], widget,
                 area, x, y, width, height);

    drawLightBevel(cr, style, state, area,
                   thinner && !horiz ? x + THIN_SBAR_MOD : x,
                   thinner && horiz ? y + THIN_SBAR_MOD : y,
                   thinner && !horiz ? width - THIN_SBAR_MOD * 2 : width,
                   thinner && horiz ? height - THIN_SBAR_MOD * 2 : height,
                   &qtcPalette.background[2], qtcPalette.background, sbarRound,
                   thinner ? WIDGET_SLIDER_TROUGH : WIDGET_TROUGH,
                   BORDER_FLAT, DF_SUNKEN | DF_DO_BORDER|
                   (horiz ? 0 : DF_VERT), widget);
}

void
drawSelectionGradient(cairo_t *cr, const QtcRect *area, int x, int y,
                      int width, int height, ECornerBits round,
                      bool isLvSelection, double alpha, const GdkColor *col,
                      bool horiz)
{
    cairo_save(cr);
    if ((!isLvSelection || !(opts.square & SQUARE_LISTVIEW_SELECTION)) &&
        opts.round != ROUND_NONE) {
        qtcCairoClipWhole(cr, x, y, width, height,
                          qtcGetRadius(&opts, width, height, WIDGET_SELECTION,
                                       RADIUS_SELECTION), round);
    }
    drawBevelGradient(cr, area, x, y, width, height, col, horiz, false,
                      opts.selectionAppearance, WIDGET_SELECTION, alpha);
    cairo_restore(cr);
}

void
drawSelection(cairo_t *cr, GtkStyle *style, GtkStateType state,
              const QtcRect *area, GtkWidget *widget, int x, int y, int width,
              int height, ECornerBits round, bool isLvSelection,
              double alphaMod, int factor)
{
    bool hasFocus = gtk_widget_has_focus(widget);
    double alpha = (alphaMod * (state == GTK_STATE_PRELIGHT ? 0.20 : 1.0) *
                    (hasFocus || !qtSettings.inactiveChangeSelectionColor ?
                     1.0 : INACTIVE_SEL_ALPHA));
    GdkColor col = style->base[hasFocus ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE];

    if (factor != 0) {
        col = shadeColor(&col, TO_FACTOR(factor));
    }
    drawSelectionGradient(cr, area, x, y, width, height, round,
                          isLvSelection, alpha, &col, true);

    if (opts.borderSelection &&
        (!isLvSelection || !(opts.square & SQUARE_LISTVIEW_SELECTION))) {
        double xd = x + 0.5;
        double yd = y + 0.5;
        double alpha = (state == GTK_STATE_PRELIGHT || alphaMod < 1.0 ?
                        0.20 : 1.0);
        int xo = x;
        int widtho = width;

        if (isLvSelection && !(opts.square & SQUARE_LISTVIEW_SELECTION) &&
            round != ROUNDED_ALL) {
            if (!(round & ROUNDED_LEFT)) {
                x -= 1;
                xd -= 1;
                width += 1;
            }
            if (!(round & ROUNDED_RIGHT)) {
                width += 1;
            }
        }

        cairo_save(cr);
        cairo_new_path(cr);
        cairo_rectangle(cr, xo, y, widtho, height);
        cairo_clip(cr);
        qtcCairoSetColor(cr, &col, alpha);
        qtcCairoPathWhole(cr, xd, yd, width - 1, height - 1,
                          qtcGetRadius(&opts, widtho, height, WIDGET_OTHER,
                                       RADIUS_SELECTION), round);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
}

void
createRoundedMask(GtkWidget *widget, int x, int y, int width,
                  int height, double radius, bool isToolTip)
{
    if (widget) {
        int size = ((width & 0xFFFF) << 16) + (height & 0xFFFF);
        QTC_DEF_WIDGET_PROPS(props, widget);
        int old = qtcWidgetProps(props)->widgetMask;

        if (size != old) {
#if GTK_CHECK_VERSION(2, 90, 0)
            GdkRegion *mask = windowMask(0, 0, width, height,
                                         opts.round > ROUND_SLIGHT);

            gtk_widget_shape_combine_region(widget, NULL);
            gtk_widget_shape_combine_region(widget, mask);
            gdk_region_destroy(mask);
#else
            GdkBitmap *mask = gdk_pixmap_new(NULL, width, height, 1);
            cairo_t *crMask = gdk_cairo_create((GdkDrawable*)mask);

            cairo_rectangle(crMask, 0, 0, width, height);
            cairo_set_source_rgba(crMask, 1, 1, 1, 0);
            cairo_set_operator(crMask, CAIRO_OPERATOR_SOURCE);
            cairo_paint(crMask);
            cairo_new_path(crMask);
            qtcCairoPathWhole(crMask, 0, 0, width, height, radius, ROUNDED_ALL);
            cairo_set_source_rgba(crMask, 0, 0, 0, 1);
            cairo_fill(crMask);
            if (isToolTip) {
                gtk_widget_shape_combine_mask(widget, mask, x, y);
            } else {
                gdk_window_shape_combine_mask(
                    gtk_widget_get_parent_window(widget), mask, 0, 0);
            }
            cairo_destroy(crMask);
            gdk_pixmap_unref(mask);
#endif
            qtcWidgetProps(props)->widgetMask = size;
            /* Setting the window type to 'popup menu' seems to
               re-eanble kwin shadows! */
            if (isToolTip && gtk_widget_get_window(widget)) {
                gdk_window_set_type_hint(gtk_widget_get_window(widget),
                                         GDK_WINDOW_TYPE_HINT_POPUP_MENU);
            }
        }
    }
}

void
clearRoundedMask(GtkWidget *widget, bool isToolTip)
{
    if (widget) {
        QTC_DEF_WIDGET_PROPS(props, widget);
        if (qtcWidgetProps(props)->widgetMask) {
#if GTK_CHECK_VERSION(2, 90, 0)
            gtk_widget_shape_combine_region(widget, NULL);
#else
            if(isToolTip)
                gtk_widget_shape_combine_mask(widget, NULL, 0, 0);
            else
                gdk_window_shape_combine_mask(gtk_widget_get_parent_window(widget), NULL, 0, 0);
#endif
            qtcWidgetProps(props)->widgetMask = 0;
        }
    }
}

void
drawTreeViewLines(cairo_t *cr, const GdkColor *col, int x, int y, int h,
                  int depth, int levelIndent, int expanderSize,
                  GtkTreeView *treeView, GtkTreePath *path)
{
    int cellIndent = levelIndent + expanderSize + 4;
    int xStart = x + cellIndent / 2;
    int isLastMask = 0;
    bool haveChildren = treeViewCellHasChildren(treeView, path);
    bool useBitMask = depth < 33;
    GByteArray *isLast = (depth && !useBitMask ?
                          g_byte_array_sized_new(depth) : NULL);

    if (useBitMask || isLast) {
        GtkTreePath  *p = path ? gtk_tree_path_copy(path) : NULL;
        int index=depth-1;

        while (p && gtk_tree_path_get_depth(p) > 0 && index>=0) {
            GtkTreePath *next=treeViewPathParent(treeView, p);
            uint8_t last=treeViewCellIsLast(treeView, p) ? 1 : 0;
            if (useBitMask) {
                if (last) {
                    isLastMask |= 1 << index;
                }
            } else {
                isLast = g_byte_array_prepend(isLast, &last, 1);
            }
            gtk_tree_path_free(p);
            p = next;
            index--;
        }
        qtcCairoSetColor(cr, col);
        for(int i = 0;i < depth;++i) {
            bool isLastCell = ((useBitMask ? isLastMask & (1 << i) :
                                isLast->data[i]) ? true : false);
            bool last = i == depth - 1;
            double xCenter = xStart;
            if (last) {
                double yCenter = (int)(y + h / 2);

                if (haveChildren) {
                    // first vertical line
                    cairo_move_to(cr, xCenter + 0.5, y);
                    // (int)(expanderSize/3));
                    cairo_line_to(cr, xCenter + 0.5, yCenter - (LV_SIZE - 1));
                    // second vertical line
                    if (!isLastCell) {
                        cairo_move_to(cr, xCenter + 0.5, y + h);
                        cairo_line_to(cr, xCenter + 0.5,
                                      yCenter + LV_SIZE + 1);
                    }

                    // horizontal line
                    cairo_move_to(cr, xCenter + (int)(expanderSize / 3) + 1,
                                  yCenter + 0.5);
                    cairo_line_to(cr, xCenter + expanderSize * 2 / 3 - 1,
                                  yCenter + 0.5);
                } else {
                    cairo_move_to(cr, xCenter + 0.5, y);
                    if(isLastCell) {
                        cairo_line_to(cr, xCenter + 0.5, yCenter);
                    } else {
                        cairo_line_to(cr, xCenter + 0.5, y + h);
                    }
                    // horizontal line
                    cairo_move_to(cr, xCenter, yCenter + 0.5);
                    cairo_line_to(cr, xCenter + expanderSize * 2 / 3 - 1,
                                  yCenter + 0.5);
                }
            } else if (!isLastCell) {
                // vertical line
                cairo_move_to(cr, xCenter + 0.5, y);
                cairo_line_to(cr, xCenter + 0.5, y + h);
            }
            cairo_stroke(cr);
            xStart += cellIndent;
        }
        if (isLast) {
            g_byte_array_free(isLast, false);
        }
    }
}

static void
qtcSetGapClip(cairo_t *cr, const QtcRect *area, GtkPositionType gapSide,
              int gapX, int gapWidth, int x, int y, int width, int height,
              bool isTab)
{
    if (gapWidth <= 0) {
        return;
    }
    QtcRect gapRect;
    int adjust = isTab ? (gapX > 1 ? 1 : 2) : 0;
    switch (gapSide) {
    case GTK_POS_TOP:
        gapRect = qtcRect(x + gapX + adjust, y, gapWidth - 2 * adjust, 2);
        if (qtSettings.app == GTK_APP_JAVA && isTab) {
            gapRect.width -= 3;
        }
        break;
    case GTK_POS_BOTTOM:
        gapRect = qtcRect(x + gapX + adjust, y + height - 2,
                          gapWidth - 2 * adjust, 2);
        break;
    case GTK_POS_LEFT:
        gapRect = qtcRect(x, y + gapX + adjust, 2, gapWidth - 2 * adjust);
        break;
    case GTK_POS_RIGHT:
        gapRect = qtcRect(x + width - 2, y + gapX + adjust,
                          2, gapWidth - 2 * adjust);
        break;
    }

    QtcRect r = {x, y, width, height};
    cairo_region_t *region = cairo_region_create_rectangle(area ? area : &r);
    cairo_region_xor_rectangle(region, &gapRect);
    qtcCairoClipRegion(cr, region);
    cairo_region_destroy(region);
}

void
fillTab(cairo_t *cr, GtkStyle *style, GtkWidget *widget, const QtcRect *area,
        GtkStateType state, const GdkColor *col, int x, int y,
        int width, int height, bool horiz, EWidget tab, bool grad)
{
    bool selected = state == GTK_STATE_NORMAL;
    bool flatBgnd = !qtcIsCustomBgnd(&opts) || opts.tabBgnd != 0;

    const GdkColor *c = col;
    GdkColor b;
    double alpha = 1.0;

    if (selected && opts.tabBgnd != 0) {
        qtcShade(col, &b, TO_FACTOR(opts.tabBgnd), opts.shading);
        c = &b;
    }

    if (!selected && (opts.bgndOpacity != 100 || opts.dlgOpacity != 100)) {
        GtkWidget *top = widget ? gtk_widget_get_toplevel(widget) : NULL;
        bool isDialog = top && GTK_IS_DIALOG(top);

        // Note: opacity is divided by 150 to make dark inactive tabs
        // more translucent
        if (isDialog && opts.dlgOpacity != 100) {
            alpha = opts.dlgOpacity / 150.0;
        } else if (!isDialog && opts.bgndOpacity != 100) {
            alpha = opts.bgndOpacity / 150.0;
        }
    }

    if (selected && opts.appearance == APPEARANCE_INVERTED) {
        if (flatBgnd) {
            qtcCairoRect(cr, area, x, y, width, height,
                         &style->bg[GTK_STATE_NORMAL], alpha);
        }
    } else if (grad) {
        drawBevelGradient(cr, area, x, y, width, height, c, horiz,
                          selected, selected ? SEL_TAB_APP : NORM_TAB_APP,
                          tab, alpha);
    } else if (!selected || flatBgnd) {
        qtcCairoRect(cr, area, x, y, width, height, c, alpha);
    }
}

static void
colorTab(cairo_t *cr, int x, int y, int width, int height, ECornerBits round,
         EWidget tab, bool horiz)
{
    cairo_pattern_t *pt =
        cairo_pattern_create_linear(x, y, horiz ? x : x + width - 1,
                                    horiz ? y + height - 1 : y);

    cairo_save(cr);
    qtcClipPath(cr, x, y, width, height, tab, RADIUS_EXTERNAL, round);
    qtcCairoPatternAddColorStop(pt, 0, &qtcPalette.highlight[ORIGINAL_SHADE],
                                tab == WIDGET_TAB_TOP ?
                                TO_ALPHA(opts.colorSelTab) : 0.0);
    qtcCairoPatternAddColorStop(pt, CAIRO_GRAD_END,
                                &qtcPalette.highlight[ORIGINAL_SHADE],
                                tab == WIDGET_TAB_TOP ? 0.0 :
                                TO_ALPHA(opts.colorSelTab));
    cairo_set_source(cr, pt);
    cairo_rectangle(cr, x, y, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(pt);
    cairo_restore(cr);
}

void
drawToolTip(cairo_t *cr, GtkWidget *widget, const QtcRect *area,
            int x, int y, int width, int height)
{
    const GdkColor *col = &qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP];

    bool nonGtk = isFakeGtk();
    bool rounded = !nonGtk && widget && !(opts.square & SQUARE_TOOLTIPS);
    bool useAlpha = (!nonGtk && qtSettings.useAlpha &&
                     isRgbaWidget(widget) && compositingActive(widget));

    if (!nonGtk && !useAlpha && GTK_IS_WINDOW(widget)) {
        gtk_window_set_opacity(GTK_WINDOW(widget), 0.875);
    }
    cairo_save(cr);
    if (rounded) {
        if (useAlpha) {
            cairo_rectangle(cr, x, y, width, height);
            cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
            cairo_set_source_rgba(cr, 0, 0, 0, 1);
            cairo_fill(cr);
            clearRoundedMask(widget, true);
        } else {
            createRoundedMask(widget, x, y, width, height,
                              opts.round >= ROUND_FULL ? 5.0 : 2.5, true);
        }
        qtcCairoClipWhole(cr, x, y, width, height,
                          opts.round >= ROUND_FULL ? 5.0 : 2.5, ROUNDED_ALL);
    }
    if (useAlpha) {
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    }
    drawBevelGradient(cr, area, x, y, width, height, col, true, false,
                      opts.tooltipAppearance, WIDGET_TOOLTIP,
                      useAlpha ? 0.875 : 1.0);
    if (!rounded && qtcIsFlat(opts.tooltipAppearance)) {
        cairo_new_path(cr);
        /*if(qtcIsFlat(opts.tooltipAppearance))*/
        qtcCairoSetColor(cr,
                         &qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP_TEXT]);
        /*else
          cairo_set_source_rgba(cr, 0, 0, 0, 0.25);*/
        cairo_rectangle(cr, x + 0.5, y + 0.5, width - 1, height - 1);
        cairo_stroke(cr);
    }
    cairo_restore(cr);
}

void
drawSplitter(cairo_t *cr, GtkStateType state, GtkStyle *style,
             const QtcRect *area, int x, int y, int width, int height)
{
    const GdkColor *cols =
        (opts.coloredMouseOver && state == GTK_STATE_PRELIGHT ?
         qtcPalette.mouseover : qtcPalette.background);

    if (state == GTK_STATE_PRELIGHT && opts.splitterHighlight) {
        GdkColor col = shadeColor(&style->bg[state],
                                  TO_FACTOR(opts.splitterHighlight));
        drawSelectionGradient(cr, area, x, y, width, height, ROUNDED_ALL,
                              false, 1.0, &col, width > height);
    }

    switch (opts.splitters) {
    case LINE_1DOT:
        qtcCairoDot(cr, x, y, width, height, &cols[QTC_STD_BORDER]);
        break;
    case LINE_NONE:
        break;
    case LINE_DOTS:
    default:
        qtcCairoDots(cr, x, y, width, height, height > width,
                     NUM_SPLITTER_DASHES, 1, area, 0, &cols[5], cols);
        break;
    case LINE_FLAT:
    case LINE_SUNKEN:
    case LINE_DASHES:
        drawLines(cr, x, y, width, height, height > width, NUM_SPLITTER_DASHES,
                  2, cols, area, 3, opts.splitters);
    }
}

void
drawSidebarButton(cairo_t *cr, GtkStateType state, GtkStyle *style,
                  const QtcRect *area, int x, int y, int width, int height)
{
    if (qtcOneOf(state, GTK_STATE_PRELIGHT, GTK_STATE_ACTIVE)) {
        bool horiz = width > height;
        const GdkColor *cols = (state == GTK_STATE_ACTIVE ?
                                qtcPalette.sidebar : qtcPalette.background);
        drawLightBevel(cr, style, state, area, x, y, width, height,
                       &cols[getFill(state, false)], cols, ROUNDED_NONE,
                       WIDGET_MENU_ITEM, BORDER_FLAT,
                       (horiz ? 0 : DF_VERT) | (state == GTK_STATE_ACTIVE ?
                                                DF_SUNKEN : 0), NULL);

        if (opts.coloredMouseOver && state == GTK_STATE_PRELIGHT) {
            const GdkColor *col = &qtcPalette.mouseover[1];
            if (horiz || opts.coloredMouseOver != MO_PLASTIK) {
                cairo_new_path(cr);
                qtcCairoSetColor(cr, col);
                cairo_move_to(cr, x, y + 0.5);
                cairo_line_to(cr, x + width - 1, y + 0.5);
                cairo_move_to(cr, x + 1, y + 1.5);
                cairo_line_to(cr, x + width - 2, y + 1.5);
                cairo_stroke(cr);
            }
            if (!horiz || opts.coloredMouseOver != MO_PLASTIK) {
                cairo_new_path(cr);
                qtcCairoSetColor(cr, col);
                cairo_move_to(cr, x + 0.5, y);
                cairo_line_to(cr, x + 0.5, y + height - 1);
                cairo_move_to(cr, x + 1.5, y + 1);
                cairo_line_to(cr, x + 1.5, y + height - 2);
                cairo_stroke(cr);
                if (opts.coloredMouseOver != MO_PLASTIK) {
                    col = &qtcPalette.mouseover[2];
                }
            }
            if (horiz || opts.coloredMouseOver != MO_PLASTIK) {
                cairo_new_path(cr);
                qtcCairoSetColor(cr, col);
                cairo_move_to(cr, x, y + height - 1.5);
                cairo_line_to(cr, x + width - 1, y + height - 1.5);
                cairo_move_to(cr, x + 1, y + height - 2.5);
                cairo_line_to(cr, x + width - 2, y + height - 2.5);
                cairo_stroke(cr);
            }
            if (!horiz || opts.coloredMouseOver != MO_PLASTIK) {
                cairo_new_path(cr);
                qtcCairoSetColor(cr, col);
                cairo_move_to(cr, x + width - 1.5, y);
                cairo_line_to(cr, x + width - 1.5, y + height - 1);
                cairo_move_to(cr, x + width - 2.5, y + 1);
                cairo_line_to(cr, x + width - 2.5, y + height - 2);
                cairo_stroke(cr);
            }
        }
    }
}

void
drawMenuItem(cairo_t *cr, GtkStateType state, GtkStyle *style,
             GtkWidget *widget, const QtcRect *area, int x, int y,
             int width, int height)
{
    GtkMenuBar *mb = isMenubar(widget, 0);
#if GTK_CHECK_VERSION(2, 90, 0) /* Gtk3:TODO !!! */
    bool active_mb = isFakeGtk() || gdk_pointer_is_grabbed();
#else
    bool active_mb = isFakeGtk() || (mb ? GTK_MENU_SHELL(mb)->active : false);

    // The handling of 'mouse pressed' in the menubar event handler doesn't
    // seem to set the menu as active, therefore the active_mb fails. However
    // the check below works...
    if (mb && !active_mb && widget)
        active_mb = widget == GTK_MENU_SHELL(mb)->active_menu_item;
#endif

    /* The following 'if' is just a hack for a menubar item problem with
       pidgin. Sometime, a 12pix width
       empty menubar item is drawn on the right - and doesnt disappear! */
    if (!mb || width > 12) {
        bool grayItem = ((!opts.colorMenubarMouseOver && mb && !active_mb &&
                          qtSettings.app != GTK_APP_OPEN_OFFICE) ||
                         !opts.useHighlightForMenu);
        const GdkColor *itemCols = (grayItem ? qtcPalette.background :
                                    qtcPalette.highlight);
        auto round = (mb ? active_mb && opts.roundMbTopOnly ? ROUNDED_TOP :
                      ROUNDED_ALL : ROUNDED_ALL);
        auto new_state = state == GTK_STATE_PRELIGHT ? GTK_STATE_NORMAL : state;
        bool stdColors = (!mb ||
                          qtcNoneOf(opts.shadeMenubars, SHADE_BLEND_SELECTED,
                                    SHADE_SELECTED));
        int fillVal = grayItem ? 4 : ORIGINAL_SHADE;
        int borderVal = opts.borderMenuitems ? 0 : fillVal;

        if (grayItem && mb && !active_mb && !opts.colorMenubarMouseOver &&
            (opts.borderMenuitems || !qtcIsFlat(opts.menuitemAppearance))) {
            fillVal = ORIGINAL_SHADE;
        }

        if (mb && !opts.roundMbTopOnly && !(opts.square & SQUARE_POPUP_MENUS)) {
            x++;
            y++;
            width -= 2;
            height -= 2;
        }
        if (grayItem && !mb &&
            (opts.lighterPopupMenuBgnd || opts.shadePopupMenu)) {
            itemCols = qtcPalette.menu;
        }
        if (!mb && opts.menuitemAppearance == APPEARANCE_FADE) {
            bool reverse = false; /* TODO !!! */
            cairo_pattern_t *pt = NULL;
            double fadePercent = 0.0;

            cairo_save(cr);
            if (opts.round != ROUND_NONE) {
                x++;
                y++;
                width -= 2;
                height -= 2;
                qtcCairoClipWhole(cr, x, y, width, height, 4,
                                  reverse ? ROUNDED_RIGHT : ROUNDED_LEFT);
            }

            fadePercent = ((double)MENUITEM_FADE_SIZE) / (double)width;
            pt = cairo_pattern_create_linear(x, y, x + width - 1, y);

            qtcCairoPatternAddColorStop(pt, 0, &itemCols[fillVal],
                                        reverse ? 0.0 : 1.0);
            qtcCairoPatternAddColorStop(pt, (reverse ? fadePercent :
                                             1.0 - fadePercent),
                                        &itemCols[fillVal]);
            qtcCairoPatternAddColorStop(pt, CAIRO_GRAD_END, &itemCols[fillVal],
                                        reverse ? 1.0 : 0.0);
            cairo_set_source(cr, pt);
            cairo_rectangle(cr, x, y, width, height);
            cairo_fill(cr);
            cairo_pattern_destroy(pt);
            cairo_restore(cr);
        } else if (!opts.borderMenuitems && !mb) {
            /* For now dont round combos - getting weird effects with
               shadow/clipping :-( ...but these work ok if we have an rgba
               colormap, so in that case we dont care if its a combo...*/
            bool isCombo = (!(opts.square & SQUARE_POPUP_MENUS) && widget &&
                            isComboMenu(gtk_widget_get_parent(widget)) &&
                            !(qtSettings.useAlpha &&
                              compositingActive(widget) && isRgbaWidget(widget)));
            bool roundedMenu = ((!widget || !isCombo) &&
                                !(opts.square & SQUARE_POPUP_MENUS));

            cairo_save(cr);
            if (roundedMenu) {
                qtcCairoClipWhole(cr, x, y, width, height,
                                  (opts.round >= ROUND_FULL ? 5.0 : 2.5) - 1.0,
                                  round);
            }
            drawBevelGradient(cr, area, x, y, width, height,
                              &itemCols[fillVal], true, false,
                              opts.menuitemAppearance, WIDGET_MENU_ITEM);
            cairo_restore(cr);
        } else if (stdColors && opts.borderMenuitems) {
            drawLightBevel(cr, style, new_state, area, x, y,
                           width, height, &itemCols[fillVal], itemCols,
                           round, WIDGET_MENU_ITEM, BORDER_FLAT,
                           DF_DRAW_INSIDE | (stdColors ? DF_DO_BORDER : 0),
                           widget);
        } else {
            if (width > 2 && height > 2)
                drawBevelGradient(cr, area, x + 1, y + 1, width - 2,height - 2,
                                  &itemCols[fillVal], true, false,
                                  opts.menuitemAppearance, WIDGET_MENU_ITEM);

            drawBorder(cr, style, state, area, x, y, width, height, itemCols,
                       round, BORDER_FLAT, WIDGET_MENU_ITEM, 0, borderVal);
        }
    }
}

void
drawMenu(cairo_t *cr, GtkWidget *widget, const QtcRect *area,
         int x, int y, int width, int height)
{
    double radius = 0.0;
    double alpha = 1.0;
    bool nonGtk = isFakeGtk();
    bool roundedMenu = !(opts.square & SQUARE_POPUP_MENUS) && !nonGtk;
    bool compsActive = compositingActive(widget);
    bool isAlphaWidget = compsActive && isRgbaWidget(widget);
    bool useAlpha = isAlphaWidget && opts.menuBgndOpacity < 100;
    bool useAlphaForCorners =
        !nonGtk && qtSettings.useAlpha && isAlphaWidget;
    bool comboMenu =
        useAlphaForCorners || !compsActive ? false : isComboMenu(widget);
    /* Cant round combos, unless using rgba - getting weird effects with
       shadow/clipping :-(. If 'useAlphaForCorners', then dont care if its a
       combo menu - as it can still be rounded */

    cairo_save(cr); // For operator
    if (useAlpha) {
        if (widget && /*!comboMenu && */opts.menuBgndOpacity != 100) {
            enableBlurBehind(widget, true);
        }
        alpha = opts.menuBgndOpacity / 100.0;
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    }
    cairo_save(cr); // For clipping
    if (roundedMenu && !comboMenu) {
        radius = opts.round >= ROUND_FULL ? 5.0 : 2.5;
        if (useAlphaForCorners) {
            cairo_save(cr);
            cairo_rectangle(cr, x, y, width, height);
            cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
            cairo_set_source_rgba(cr, 0, 0, 0, 1);
            cairo_fill(cr);
            cairo_restore(cr);
            clearRoundedMask(widget, false);
        } else {
            createRoundedMask(widget, x, y, width, height,
                              radius - 0.25, false);
        }
        qtcCairoClipWhole(cr, x, y, width, height, radius, ROUNDED_ALL);
    }

    if (!qtcIsFlatBgnd(opts.menuBgndAppearance)) {
        if (opts.menuBgndAppearance == APPEARANCE_STRIPED) {
            drawStripedBgnd(cr, x, y, width, height,
                            &qtcPalette.menu[ORIGINAL_SHADE], alpha);
        } else if (opts.menuBgndAppearance == APPEARANCE_FILE) {
            drawBgndImage(cr, x, y, width, height, false);
        } else {
            drawBevelGradient(cr, area, x, y, width, height,
                              &qtcPalette.menu[ORIGINAL_SHADE],
                              opts.menuBgndGrad == GT_HORIZ, false,
                              opts.menuBgndAppearance, WIDGET_OTHER, alpha);
        }
    } else if (opts.shadePopupMenu || opts.lighterPopupMenuBgnd || useAlpha) {
        qtcCairoRect(cr, area, x, y, width, height,
                     &qtcPalette.menu[ORIGINAL_SHADE], alpha);
    }
    if (opts.menuBgndImage.type != IMG_NONE) {
        drawBgndRings(cr, x, y, width, height, false);
    }
    if (opts.menuStripe && !comboMenu) {
        bool mozOo = isFakeGtk();
        int stripeWidth = mozOo ? 22 : 21;

        // To determine stripe size, we iterate over all menuitems of this menu.
        // If we find a GtkImageMenuItem then we can a width of 20. However, we
        // need to check that at least one enttry actually has an image! So, if
        // the first GtkImageMenuItem has an image then we're ok, otherwise we
        // give it a blank pixmap.
        if (!mozOo && widget) {
            GtkMenuShell *menuShell = GTK_MENU_SHELL(widget);
            GList *children =
                gtk_container_get_children(GTK_CONTAINER(menuShell));
            for (GList *child = children;child;child = child->next) {
                if (GTK_IS_IMAGE_MENU_ITEM(child->data)) {
                    GtkImageMenuItem *item = GTK_IMAGE_MENU_ITEM(child->data);
                    stripeWidth = 21;
                    if (!gtk_image_menu_item_get_image(item) ||
                        (GTK_IS_IMAGE(gtk_image_menu_item_get_image(item)) &&
                        gtk_image_get_storage_type(
                            GTK_IMAGE(gtk_image_menu_item_get_image(item))) ==
                         GTK_IMAGE_EMPTY)) {
                        // Give it a blank icon - so that menuStripe looks ok,
                        // plus this matches KDE style!
                        if (!gtk_image_menu_item_get_image(item)) {
                            gtk_image_menu_item_set_image(
                                item, gtk_image_new_from_pixbuf(
                                    getPixbuf(qtcPalette.check_radio,
                                              PIX_BLANK, 1.0)));
                        } else {
                            gtk_image_set_from_pixbuf(
                                GTK_IMAGE(gtk_image_menu_item_get_image(item)),
                                getPixbuf(qtcPalette.check_radio,
                                          PIX_BLANK, 1.0));
                        }
                        break;
                    } else {
                        // TODO: Check image size!
                        break;
                    }
                }
            }
            if (children) {
                g_list_free(children);
            }
        }

        drawBevelGradient(cr, area, x + 1, y + 1, stripeWidth + 1,
                          height - 2, &opts.customMenuStripeColor, false, false,
                          opts.menuStripeAppearance, WIDGET_OTHER, alpha);
    }

    cairo_restore(cr); // For clipping
    if (opts.popupBorder) {
        EGradientBorder border =
            qtcGetGradient(opts.menuBgndAppearance, &opts)->border;
        cairo_new_path(cr);
        qtcCairoSetColor(cr, &qtcPalette.menu[QTC_STD_BORDER]);
        /* For now dont round combos - getting weird effects with
         * shadow/clipping :-( */
        if (roundedMenu && !comboMenu) {
            qtcCairoPathWhole(cr, x + 0.5, y + 0.5, width - 1, height - 1,
                              radius - 1, ROUNDED_ALL);
        } else {
            cairo_rectangle(cr, x + 0.5, y + 0.5, width - 1, height - 1);
        }
        cairo_stroke(cr);
        if (qtcUseBorder(border) &&
            opts.menuBgndAppearance != APPEARANCE_FLAT) {
            if (roundedMenu) {
                if (border != GB_3D) {
                    cairo_new_path(cr);
                    qtcCairoSetColor(cr, qtcPalette.menu);
                    qtcCairoPathTopLeft(cr, x + 1.5, y + 1.5, width - 3,
                                        height - 3, radius - 2, ROUNDED_ALL);
                    cairo_stroke(cr);
                }
                cairo_new_path(cr);
                qtcCairoSetColor(cr, &qtcPalette.menu[border == GB_LIGHT ? 0 :
                                                      FRAME_DARK_SHADOW]);
                qtcCairoPathBottomRight(cr, x + 1.5, y + 1.5, width - 3,
                                        height - 3, radius - 2, ROUNDED_ALL);
                cairo_stroke(cr);
            } else {
                if (border != GB_3D) {
                    qtcCairoHLine(cr, x + 1, y + 1, width - 2, qtcPalette.menu);
                    qtcCairoVLine(cr, x + 1, y + 1, height - 2,
                                  qtcPalette.menu);
                }
                qtcCairoHLine(cr, x + 1, y + height - 2, width - 2,
                              &qtcPalette.menu[border == GB_LIGHT ? 0 :
                                               FRAME_DARK_SHADOW]);
                qtcCairoVLine(cr, x + width - 2, y + 1, height - 2,
                              &qtcPalette.menu[border == GB_LIGHT ? 0 :
                                               FRAME_DARK_SHADOW]);
            }
        }
    }
    cairo_restore(cr); // For operator
}

void
drawBoxGap(cairo_t *cr, GtkStyle *style, GtkShadowType shadow,
           GtkStateType state, GtkWidget *widget, const QtcRect *area,
           int x, int y, int width, int height, GtkPositionType gapSide,
           int gapX, int gapWidth, EBorder borderProfile, bool isTab)
{
    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %d %d %d %d ", __FUNCTION__,
               shadow, state, x, y, width, height, gapX, gapWidth, isTab);
        debugDisplayWidget(widget, 10);
    }

    // *Very* hacky fix for tabs in thunderbird main window!!!
    if (isTab && isMozilla() && gapWidth == 250 &&
        (width == 290 || width == 270) && height == 6) {
        return;
    }
    if (isTab && opts.tabBgnd != 0) {
        cairo_save(cr);
        qtcClipPath(cr, x - 1, y - 1, width + 2, height + 2,
                    WIDGET_TAB_FRAME, RADIUS_EXTERNAL, ROUNDED_ALL);
        drawAreaMod(cr, style, state, area, TO_FACTOR(opts.tabBgnd),
                    x, y, width, height);
        cairo_restore(cr);
    }
    if (opts.tabMouseOver == TAB_MO_GLOW && gapWidth > 4 &&
        isMozillaWidget(widget)) {
        gapWidth -= 2;
    }
    if (shadow != GTK_SHADOW_NONE) {
        auto round = (((!isTab && opts.square & SQUARE_FRAME) ||
                       (isTab && opts.square & SQUARE_TAB_FRAME)) ?
                      ROUNDED_NONE : ROUNDED_ALL);
        GtkWidget *parent = widget ? gtk_widget_get_parent(widget) : NULL;

        if (!(opts.square & SQUARE_TAB_FRAME) && gapX <= 0) {
            switch (gapSide) {
            case GTK_POS_TOP:
                round = ECornerBits(CORNER_TR | CORNER_BL | CORNER_BR);
                break;
            case GTK_POS_BOTTOM:
                round = ECornerBits(CORNER_BR | CORNER_TL | CORNER_TR);
                break;
            case GTK_POS_LEFT:
                round = ECornerBits(CORNER_TR | CORNER_BL | CORNER_BR);
                break;
            case GTK_POS_RIGHT:
                round = ECornerBits(CORNER_TL | CORNER_BL | CORNER_BR);
                break;
            }
        }
        cairo_save(cr);
        qtcSetGapClip(cr, area, gapSide, gapX, gapWidth, x, y,
                      width, height, isTab);
        drawBorder(cr, gtk_widget_get_style(parent ? parent : widget), state,
                   area, x, y, width, height, NULL, round,
                   borderProfile, isTab ? WIDGET_TAB_FRAME : WIDGET_FRAME,
                   isTab ? 0 : DF_BLEND);
        cairo_restore(cr);
    }
}

void
drawBoxGapFixes(cairo_t *cr, GtkWidget *widget,  int x, int y,
                int width, int height, GtkPositionType gapSide, int gapX,
                int gapWidth)
{
    const GdkColor *col1 = &qtcPalette.background[0];
    const GdkColor *col2 =
        &qtcPalette.background[opts.borderTab ? 0 :
                               (opts.appearance == APPEARANCE_FLAT ?
                                ORIGINAL_SHADE : FRAME_DARK_SHADOW)];
    const GdkColor *outer = &qtcPalette.background[QTC_STD_BORDER];
    bool rev = reverseLayout(widget);
    bool thin = opts.thin & THIN_FRAMES;
    int rightPos = width - (gapX + gapWidth);

    switch (gapSide) {
    case GTK_POS_TOP:
        if (gapX > 0) {
            if (!thin) {
                qtcCairoHLine(cr, x + gapX - 1, y + 1, 3, col1);
                qtcCairoHLine(cr, x + gapX - 1, y, 3, col1);
            }
            qtcCairoHLine(cr, x + gapX - 1, y, 2, outer);
        } else if (!thin) {
            qtcCairoVLine(cr, x + 1, y, 2, col1);
        }
        if (rightPos >= 0) {
            if (!thin) {
                qtcCairoHLine(cr, x + gapX + gapWidth - 2, y + 1, 3, col1);
                qtcCairoVLine(cr, x + gapX + gapWidth - 2,
                              y, rightPos ? 1 : 0, col2);
            }
            qtcCairoHLine(cr, x + gapX + gapWidth - 1, y, 2, outer);
        }
        if (!(opts.square & SQUARE_TAB_FRAME) && opts.round > ROUND_SLIGHT) {
            if (gapX > 0 && opts.tabMouseOver == TAB_MO_GLOW) {
                qtcCairoVLine(cr, rev ? x + width - 2 : x + 1, y, 2, outer);
            } else {
                qtcCairoVLine(cr, rev ? x + width - 1 : x, y, 3, outer);
                if (gapX > 0 && !thin) {
                    qtcCairoHLine(cr, x + 1, y, 1, &qtcPalette.background[2]);
                }
            }
        }
        break;
    case GTK_POS_BOTTOM:
        if (gapX > 0) {
            if (!thin) {
                qtcCairoHLine(cr, x + gapX - 1, y + height - 1, 2, col1);
                qtcCairoHLine(cr, x + gapX - 1, y + height - 2, 2, col2);
            }
            qtcCairoHLine(cr, x + gapX - 1, y + height - 1, 2, outer);
        } else if (!thin) {
            qtcCairoVLine(cr, x + 1, y + height - 1, 2, col1);
        }
        if (rightPos >= 0) {
            if (!thin) {
                qtcCairoHLine(cr, x + gapX + gapWidth - 2,
                              y + height - 2, 3, col2);
                qtcCairoVLine(cr, x + gapX + gapWidth - 2,
                              y + height - 1, rightPos ? 1 : 0, col2);
            }
            qtcCairoHLine(cr, x + gapX + gapWidth - 1,
                          y + height - 1, 2, outer);
        }
        if (!(opts.square & SQUARE_TAB_FRAME) && opts.round > ROUND_SLIGHT) {
            if (gapX > 0 && opts.tabMouseOver == TAB_MO_GLOW) {
                qtcCairoVLine(cr, rev ? x + width - 2 : x + 1,
                              y + height - 2, 2, outer);
            } else {
                qtcCairoVLine(cr, rev ? x + width - 1 : x,
                              y + height - 3, 3, outer);
            }
        }
        break;
    case GTK_POS_LEFT:
        if (gapX > 0) {
            if (!thin) {
                qtcCairoVLine(cr, x + 1, y + gapX - 1, 3, col1);
                qtcCairoVLine(cr, x, y + gapX - 1, 3, col1);
            }
            qtcCairoVLine(cr, x, y + gapX - 1, 2, outer);
        } else if (!thin) {
            qtcCairoHLine(cr, x, y + 1, 2, col1);
        }
        if (height - (gapX + gapWidth) > 0) {
            if (!thin) {
                qtcCairoVLine(cr, x + 1, y + gapX + gapWidth - 2, 3, col1);
                qtcCairoVLine(cr, x, y + gapX + gapWidth - 2, 1, col2);
            }
            qtcCairoVLine(cr, x, y + gapX + gapWidth - 1, 2, outer);
        }
        if(!(opts.square & SQUARE_TAB_FRAME) && opts.round > ROUND_SLIGHT) {
            if (gapX > 0 && opts.tabMouseOver == TAB_MO_GLOW) {
                qtcCairoHLine(cr, x, y + 1, 2, outer);
            } else {
                qtcCairoHLine(cr, x, y, 3, outer);
                if (gapX > 0 && !thin) {
                    qtcCairoHLine(cr, x, y + 1, 1, &qtcPalette.background[2]);
                }
            }
        }
        break;
    case GTK_POS_RIGHT:
        if (gapX > 0) {
            if (!thin)
                qtcCairoVLine(cr, x + width - 2, y + gapX - 1, 2, col2);
            qtcCairoVLine(cr, x + width - 1, y + gapX - 1, 2, outer);
        } else if (!thin) {
            qtcCairoHLine(cr, x + width - 2, y + 1, 3, col1);
        }

        if (height - (gapX + gapWidth) > 0) {
            if (!thin) {
                qtcCairoHLine(cr, x + width - 2,
                              y + gapX + gapWidth - 2, 3, col2);
                qtcCairoVLine(cr, x + width - 2,
                              y + gapX + gapWidth - 1, 2, col2);
            }
            qtcCairoVLine(cr, x + width - 1, y + gapX + gapWidth - 1, 2, outer);
        }
        if (!(opts.square & SQUARE_TAB_FRAME) && opts.round > ROUND_SLIGHT) {
            if (gapX > 0 && opts.tabMouseOver == TAB_MO_GLOW) {
                qtcCairoHLine(cr, x + width - 2, y + 1, 2, outer);
            } else {
                qtcCairoHLine(cr, x + width - 3, y, 3, outer);
            }
        }
        break;
    }
}

void
drawShadowGap(cairo_t *cr, GtkStyle *style, GtkShadowType shadow,
              GtkStateType state, GtkWidget *widget, const QtcRect *area,
              int x, int y, int width, int height, GtkPositionType gapSide,
              int gapX, int gapWidth)
{
    bool drawFrame = true;
    bool isGroupBox = IS_GROUP_BOX(widget);

    if (isGroupBox) {
        if (gapX < 5) {
            gapX += 5;
            gapWidth += 2;
        }
        switch (opts.groupBox) {
        case FRAME_NONE:
            drawFrame = false;
            return;
        case FRAME_LINE:
        case FRAME_SHADED:
        case FRAME_FADED:
            if (opts.gbLabel & (GB_LBL_INSIDE | GB_LBL_OUTSIDE) && widget &&
                GTK_IS_FRAME(widget)) {
                GtkFrame *frame = GTK_FRAME(widget);
                GtkRequisition child_requisition;
                int height_extra;
                GtkStyle *style =
                    (frame ? gtk_widget_get_style(GTK_WIDGET(frame)) : NULL);
                GtkWidget *label =
                    (frame ? gtk_frame_get_label_widget(frame) : NULL);

                if (style && label) {
                    gtk_widget_get_child_requisition(label, &child_requisition);
                    height_extra =
                        (qtcMax(0, child_requisition.height -
                                style->ythickness) -
                         qtcFrameGetLabelYAlign(frame) *
                         child_requisition.height) + 2;

                    if (opts.gbLabel & GB_LBL_INSIDE) {
                        y -= height_extra;
                        height += height_extra;
                        gapWidth = 0;
                    } else if (opts.gbLabel & GB_LBL_OUTSIDE) {
                        y += height_extra;
                        height -= height_extra;
                        gapWidth = 0;
                    }
                }
            }
            if (opts.groupBox == FRAME_LINE) {
                QtcRect gap = {x, y, gapWidth, 1};
                drawFadedLine(cr, x, y, width, 1,
                              &qtcPalette.background[QTC_STD_BORDER],
                              area, gapWidth > 0 ? &gap : NULL,
                              false, true, true);
                drawFrame = false;
            } else if (shadow != GTK_SHADOW_NONE) {
                auto round = (opts.square & SQUARE_FRAME ?
                              ROUNDED_NONE : ROUNDED_ALL);
                double col = opts.gbFactor < 0 ? 0.0 : 1.0;
                double radius =
                    (round == ROUNDED_ALL ?
                     qtcGetRadius(&opts, width, height, WIDGET_FRAME,
                                  RADIUS_EXTERNAL) : 0.0);
                if (opts.gbFactor != 0) {
                    cairo_save(cr);
                    qtcCairoClipWhole(cr, x + 0.5, y + 0.5, width - 1,
                                      height - 1, radius, round);
                    cairo_rectangle(cr, x, y, width, height);
                    if (opts.groupBox == FRAME_SHADED) {
                        cairo_set_source_rgba(cr, col, col, col,
                                              TO_ALPHA(opts.gbFactor));
                    } else {
                        cairo_pattern_t *pt =
                            cairo_pattern_create_linear(x, y, x,
                                                        y + height - 1);
                        cairo_pattern_add_color_stop_rgba(
                            pt, 0, col, col, col, TO_ALPHA(opts.gbFactor));
                        cairo_pattern_add_color_stop_rgba(
                            pt, CAIRO_GRAD_END, col, col, col, 0);
                        cairo_set_source(cr, pt);
                        cairo_pattern_destroy(pt);
                    }
                    cairo_fill(cr);
                    cairo_restore(cr);
                }
                if (opts.groupBox == FRAME_FADED) {
                    cairo_pattern_t *pt =
                        cairo_pattern_create_linear(x, y, x, y + height - 1);
                    qtcCairoPatternAddColorStop(
                        pt, 0, &qtcPalette.background[QTC_STD_BORDER]);
                    qtcCairoPatternAddColorStop(
                        pt, CAIRO_GRAD_END,
                        &qtcPalette.background[QTC_STD_BORDER], 0);
                    cairo_save(cr);
                    qtcSetGapClip(cr, area, gapSide, gapX, gapWidth,
                                  x, y, width, height, false);
                    cairo_set_source(cr, pt);
                    qtcCairoPathWhole(cr, x + 0.5, y + 0.5, width - 1,
                                      height - 1, radius, round);
                    cairo_stroke(cr);
                    cairo_restore(cr);
                    cairo_pattern_destroy(pt);
                    drawFrame = false;
                }
            }
            break;
        default:
            break;
        }
    }
    if (drawFrame) {
        drawBoxGap(cr, style, shadow, state, widget, area, x, y,
                   width, height, gapSide, gapX, gapWidth,
                   isGroupBox && opts.groupBox == FRAME_SHADED &&
                   shadow != GTK_SHADOW_NONE ? BORDER_SUNKEN :
                   shadowToBorder(shadow), false);
    }
}

void
drawCheckBox(cairo_t *cr, GtkStateType state, GtkShadowType shadow,
             GtkStyle *style, GtkWidget *widget, const char *detail,
             const QtcRect *area, int x, int y, int width, int height)
{
    if (state == GTK_STATE_PRELIGHT &&
        qtcOneOf(qtSettings.app, GTK_APP_MOZILLA, GTK_APP_JAVA)) {
        state = GTK_STATE_NORMAL;
    }
    bool mnu = DETAIL("check");
    bool list = !mnu && isList(widget);
    bool on = shadow == GTK_SHADOW_IN;
    bool tri = shadow == GTK_SHADOW_ETCHED_IN;
    bool doEtch = opts.buttonEffect != EFFECT_NONE;
    GdkColor newColors[TOTAL_SHADES + 1];
    const GdkColor *btnColors;
    auto ind_state = ((list || (!mnu && state == GTK_STATE_INSENSITIVE)) ?
                      state : GTK_STATE_NORMAL);
    int checkSpace = doEtch ? opts.crSize + 2 : opts.crSize;

    if (opts.crColor && state != GTK_STATE_INSENSITIVE && (on || tri)) {
        btnColors = qtcPalette.selectedcr;
    } else if (!mnu && !list && QT_CUSTOM_COLOR_BUTTON(style)) {
        qtcShadeColors(&style->bg[state], newColors);
        btnColors = newColors;
    } else {
        btnColors = qtcPalette.button[state == GTK_STATE_INSENSITIVE ?
                                      PAL_DISABLED : PAL_ACTIVE];
    }
    x += (width - checkSpace) / 2;
    y += (height - checkSpace) / 2;
    if (qtSettings.debug == DEBUG_ALL) {
        printf(DEBUG_PREFIX "%s %d %d %d %d %d %d %d %s  ",
               __FUNCTION__, state, shadow, x, y, width, height, mnu,
               detail ? detail : "NULL");
        debugDisplayWidget(widget, 10);
    }
    if ((mnu && state == GTK_STATE_PRELIGHT) ||
        (list && state == GTK_STATE_ACTIVE)) {
        state = GTK_STATE_NORMAL;
    }
    if (mnu && isMozilla()) {
        x -= 2;
    }
    if (!mnu || qtSettings.qt4) {
        if (opts.crButton) {
            drawLightBevel(cr, style, state, area, x, y, checkSpace,
                           checkSpace, &btnColors[getFill(state, false)],
                           btnColors, ROUNDED_ALL, WIDGET_CHECKBOX, BORDER_FLAT,
                           DF_DO_BORDER |
                           (state == GTK_STATE_ACTIVE ? DF_SUNKEN : 0),
                           list ? NULL : widget);
            if (doEtch) {
                x++;
                y++;
            }
        } else {
            bool coloredMouseOver =
                state == GTK_STATE_PRELIGHT && opts.coloredMouseOver;
            bool glow = (doEtch && state == GTK_STATE_PRELIGHT &&
                         opts.coloredMouseOver == MO_GLOW);
            bool lightBorder = DRAW_LIGHT_BORDER(false, WIDGET_TROUGH,
                                                 APPEARANCE_INVERTED);
            const GdkColor *colors =
                coloredMouseOver ? qtcPalette.mouseover : btnColors;
            const GdkColor *bgndCol =
                (qtcOneOf(state, GTK_STATE_INSENSITIVE, GTK_STATE_ACTIVE) ?
                 &style->bg[GTK_STATE_NORMAL] :
                 !mnu && state == GTK_STATE_PRELIGHT && !coloredMouseOver &&
                 !opts.crHighlight ? &btnColors[CR_MO_FILL] :
                 &style->base[GTK_STATE_NORMAL]);
            if (doEtch) {
                x++;
                y++;
            }

            drawBevelGradient(cr, area, x + 1, y + 1, opts.crSize - 2,
                              opts.crSize - 2, bgndCol, true, false,
                              APPEARANCE_INVERTED, WIDGET_TROUGH);

            if (coloredMouseOver && !glow) {
                cairo_new_path(cr);
                qtcCairoSetColor(cr, &colors[CR_MO_FILL]);
                cairo_rectangle(cr, x + 1.5, y + 1.5, opts.crSize - 3,
                                opts.crSize - 3);
                /* cairo_rectangle(cr, x + 2.5, y + 2.5, opts.crSize - 5, */
                /*                 opts.crSize - 5); */
                cairo_stroke(cr);
            } else {
                cairo_new_path(cr);
                if (lightBorder) {
                    qtcCairoSetColor(
                        cr, &btnColors[LIGHT_BORDER(APPEARANCE_INVERTED)]);
                    cairo_rectangle(cr, x + 1.5, y + 1.5, opts.crSize - 3,
                                    opts.crSize - 3);
                } else {
                    GdkColor mid = midColor(state == GTK_STATE_INSENSITIVE ?
                                            &style->bg[GTK_STATE_NORMAL] :
                                            &style->base[GTK_STATE_NORMAL],
                                            &colors[3]);
                    qtcCairoSetColor(cr, &mid);
                    cairo_move_to(cr, x + 1.5, y + opts.crSize - 1.5);
                    cairo_line_to(cr, x + 1.5, y + 1.5);
                    cairo_line_to(cr, x + opts.crSize - 1.5, y + 1.5);
                }
                cairo_stroke(cr);
            }
            if (doEtch && (!list || glow) && !mnu) {
                if(glow && !(opts.thin & THIN_FRAMES)) {
                    drawGlow(cr, area, x - 1, y - 1, opts.crSize + 2,
                             opts.crSize + 2, ROUNDED_ALL, WIDGET_CHECKBOX);
                } else {
                    drawEtch(cr, area, widget, x - 1, y - 1,
                             opts.crSize + 2, opts.crSize + 2, false,
                             ROUNDED_ALL, WIDGET_CHECKBOX);
                }
            }
            drawBorder(cr, style, state, area, x, y, opts.crSize,
                       opts.crSize, colors, ROUNDED_ALL, BORDER_FLAT,
                       WIDGET_CHECKBOX, 0);
        }
    }

    if (on) {
        GdkPixbuf *pix = getPixbuf(getCheckRadioCol(style, ind_state, mnu),
                                   PIX_CHECK, 1.0);
        int pw = gdk_pixbuf_get_width(pix);
        int ph = gdk_pixbuf_get_height(pix);
        int dx = x + opts.crSize / 2 - pw / 2;
        int dy = y + opts.crSize / 2 - ph/2;

        gdk_cairo_set_source_pixbuf(cr, pix, dx, dy);
        cairo_paint(cr);
    } else if (tri) {
        int ty = y + opts.crSize / 2;
        const GdkColor *col = getCheckRadioCol(style, ind_state, mnu);

        qtcCairoHLine(cr, x + 3, ty, opts.crSize - 6, col);
        qtcCairoHLine(cr, x + 3, ty + 1, opts.crSize - 6, col);
    }
}

void
drawRadioButton(cairo_t *cr, GtkStateType state, GtkShadowType shadow,
                GtkStyle *style, GtkWidget *widget, const char *detail,
                const QtcRect *area, int x, int y, int width, int height)
{
    if (state == GTK_STATE_PRELIGHT &&
        qtcOneOf(qtSettings.app, GTK_APP_MOZILLA, GTK_APP_JAVA)) {
        state = GTK_STATE_NORMAL;
    }
    bool mnu = DETAIL("option");
    bool list = !mnu && isList(widget);
    if ((mnu && state == GTK_STATE_PRELIGHT) ||
        (list && state == GTK_STATE_ACTIVE)) {
        state = GTK_STATE_NORMAL;
    }

    if (!qtSettings.qt4 && mnu) {
        drawCheckBox(cr, state, shadow, style, widget, "check", area,
                     x, y, width, height);
    } else {
        bool on = shadow == GTK_SHADOW_IN;
        bool tri = shadow == GTK_SHADOW_ETCHED_IN;
        bool doEtch = opts.buttonEffect != EFFECT_NONE;
        auto ind_state = (state == GTK_STATE_INSENSITIVE ?
                          state : GTK_STATE_NORMAL);
        int optSpace = doEtch ? opts.crSize + 2 : opts.crSize;
        GdkColor newColors[TOTAL_SHADES + 1];
        const GdkColor *btnColors;
        x += (width - optSpace) / 2;
        y += (height - optSpace) / 2;
        if (opts.crColor && state != GTK_STATE_INSENSITIVE && (on || tri)) {
            btnColors = qtcPalette.selectedcr;
        } else if (!mnu && !list && QT_CUSTOM_COLOR_BUTTON(style)) {
            qtcShadeColors(&style->bg[state], newColors);
            btnColors = newColors;
        } else {
            btnColors = qtcPalette.button[state == GTK_STATE_INSENSITIVE ?
                                          PAL_DISABLED : PAL_ACTIVE];
        }
        if (opts.crButton) {
            drawLightBevel(cr, style, state, area, x, y, optSpace,
                           optSpace, &btnColors[getFill(state, false)],
                           btnColors, ROUNDED_ALL, WIDGET_RADIO_BUTTON,
                           BORDER_FLAT, DF_DO_BORDER |
                           (state == GTK_STATE_ACTIVE ? DF_SUNKEN : 0),
                           list ? NULL : widget);
            if (doEtch) {
                x++;
                y++;
            }
        } else {
            bool glow = (doEtch && state == GTK_STATE_PRELIGHT &&
                         opts.coloredMouseOver == MO_GLOW);
            bool lightBorder = DRAW_LIGHT_BORDER(false, WIDGET_TROUGH,
                                                 APPEARANCE_INVERTED);
            bool coloredMouseOver = (state == GTK_STATE_PRELIGHT &&
                                     opts.coloredMouseOver);
            bool doneShadow = false;
            const GdkColor *colors = (coloredMouseOver ?
                                      qtcPalette.mouseover : btnColors);
            const GdkColor *bgndCol =
                (qtcOneOf(state, GTK_STATE_INSENSITIVE, GTK_STATE_ACTIVE) ?
                 &style->bg[GTK_STATE_NORMAL] :
                 !mnu && GTK_STATE_PRELIGHT == state &&
                 !coloredMouseOver && !opts.crHighlight ?
                 &colors[CR_MO_FILL] : &style->base[GTK_STATE_NORMAL]);
            double radius = (opts.crSize + 1) / 2.0;

            if (doEtch) {
                x++;
                y++;
            }
            cairo_save(cr);
            qtcClipPath(cr, x, y, opts.crSize, opts.crSize,
                        WIDGET_RADIO_BUTTON, RADIUS_EXTERNAL, ROUNDED_ALL);
            drawBevelGradient(cr, NULL, x + 1, y + 1, opts.crSize - 2,
                              opts.crSize - 2, bgndCol,true, false,
                              APPEARANCE_INVERTED, WIDGET_TROUGH);
            cairo_restore(cr);
            if (!mnu && coloredMouseOver && !glow) {
                double radius = (opts.crSize - 2) / 2.0;
                qtcCairoSetColor(cr, &colors[CR_MO_FILL]);
                cairo_arc(cr, x + radius + 1, y+radius + 1,
                          radius, 0, 2 * M_PI);
                cairo_stroke(cr);
                radius--;
                cairo_arc(cr, x + radius + 2, y + radius + 2,
                          radius, 0, 2 * M_PI);
                cairo_stroke(cr);
            }
            if (!doneShadow && doEtch && !mnu && (!list || glow)) {
                double radius = (opts.crSize + 1) / 2.0;
                if (glow) {
                    qtcCairoSetColor(cr, &qtcPalette.mouseover[GLOW_MO]);
                } else {
                    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0,
                                          ETCH_RADIO_TOP_ALPHA);
                }
                if (opts.buttonEffect != EFFECT_NONE || glow) {
                    cairo_arc(cr, x + radius - 0.5, y + radius - 0.5,
                              radius, 0.75 * M_PI, 1.75 * M_PI);
                    cairo_stroke(cr);
                    if (!glow) {
                        setLowerEtchCol(cr, widget);
                    }
                }
                cairo_arc(cr, x + radius - 0.5, y + radius - 0.5,
                          radius, 1.75 * M_PI, 0.75 * M_PI);
                cairo_stroke(cr);
            }
            qtcCairoSetColor(
                cr, &colors[coloredMouseOver ? 4 :
                            BORDER_VAL(state != GTK_STATE_INSENSITIVE)]);
            radius = (opts.crSize - 0.5)/2.0;
            cairo_arc(cr, x + 0.25 + radius, y + 0.25 + radius,
                      radius, 0, 2 * M_PI);
            cairo_stroke(cr);
            if (!coloredMouseOver) {
                radius = (opts.crSize - 1) / 2.0;
                qtcCairoSetColor(cr, &btnColors[coloredMouseOver ? 3 : 4]);
                cairo_arc(cr, x + 0.75 + radius, y + 0.75 + radius, radius,
                          lightBorder ? 0 : 0.75 * M_PI,
                          lightBorder ? 2 * M_PI : 1.75 * M_PI);
                cairo_stroke(cr);
            }
        }
        if (on) {
            const GdkColor *col = getCheckRadioCol(style, ind_state, mnu);
            double radius = opts.smallRadio ? 2.5 : 3.5;
            double offset = opts.crSize / 2.0 - radius;

            qtcCairoSetColor(cr, col);
            cairo_arc(cr, x + offset + radius, y + offset + radius,
                      radius, 0, 2 * M_PI);
            cairo_fill(cr);
        } else if (tri) {
            int ty = y + opts.crSize / 2;
            const GdkColor *col = getCheckRadioCol(style, ind_state, mnu);

            qtcCairoHLine(cr, x + 3, ty, opts.crSize - 6, col);
            qtcCairoHLine(cr, x + 3, ty + 1, opts.crSize - 6, col);
        }
    }
}

void
drawTab(cairo_t *cr, GtkStateType state, GtkStyle *style, GtkWidget *widget,
        QtcRect *area, int x, int y, int width, int height,
        GtkPositionType gapSide)
{
    GtkNotebook *notebook =
        GTK_IS_NOTEBOOK(widget) ? GTK_NOTEBOOK(widget) : NULL;
    bool highlightingEnabled = notebook && (opts.highlightFactor ||
                                            opts.coloredMouseOver);
    bool highlight = false;
    int highlightedTabIndex = qtcTabCurrentHoveredIndex(widget);
    int moOffset = ((opts.round == ROUND_NONE ||
                     opts.tabMouseOver != TAB_MO_TOP) ? 1 : opts.round);
    GtkWidget *parent = widget ? gtk_widget_get_parent(widget) : NULL;
    bool firstTab = !notebook;
    bool lastTab = !notebook;
    bool vertical = qtcOneOf(gapSide, GTK_POS_LEFT, GTK_POS_RIGHT);
    bool active = state == GTK_STATE_NORMAL; /* Normal -> active tab? */
    bool rev = (qtcOneOf(gapSide, GTK_POS_TOP, GTK_POS_BOTTOM) && parent &&
                reverseLayout(parent));
    bool mozTab = isMozillaTab(widget);
    bool glowMo = (!active && notebook && opts.coloredMouseOver &&
                   opts.tabMouseOver == TAB_MO_GLOW);
    bool drawOuterGlow = glowMo && !(opts.thin & THIN_FRAMES);
    int mod = active ? 1 : 0;
    int highlightOffset = (opts.highlightTab &&
                           opts.round > ROUND_SLIGHT ? 2 : 1);
    int highlightBorder = opts.round > ROUND_FULL ? 4 : 3;
    int sizeAdjust = ((!active || mozTab) &&
                      opts.tabMouseOver == TAB_MO_GLOW ? 1 : 0);
    int tabIndex = -1;
    const GdkColor *col = (active ? &style->bg[GTK_STATE_NORMAL] :
                           &qtcPalette.background[2]);
    const GdkColor *selCol1 = &qtcPalette.highlight[0];
    QtcRect clipArea;
    EBorder borderProfile = (active || opts.borderInactiveTab ? opts.borderTab ?
                             BORDER_LIGHT : BORDER_RAISED : BORDER_FLAT);

#if !GTK_CHECK_VERSION(2, 90, 0)
    /* Hacky fix for tabs in Thunderbird */
    if (mozTab && area && area->x < x - 10) {
        return;
    }
#endif

    /* f'in mozilla apps dont really use Gtk widgets - they just paint to a
       pixmap. So, no way of knowing the position of a tab! The 'best' look
       seems to be to round both corners. Not nice, but... */
    if (mozTab || qtSettings.app == GTK_APP_JAVA) {
        firstTab = lastTab = true;
    } else if (notebook) {
        /* Borrowed from Qt engine... */
        GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
        int num_children = children ? g_list_length(children) : 0;
        int sdiff = 10000;
        int first_shown = -1;
        int last_shown = -1;
        if (children) {
            g_list_free(children);
        }
        for (int i = 0; i < num_children;i++) {
            GtkWidget *page = gtk_notebook_get_nth_page(notebook, i);
            GtkWidget *tab_label = gtk_notebook_get_tab_label(notebook, page);
            int diff = -1;

            if (tab_label) {
                QtcRect alloc = qtcWidgetGetAllocation(tab_label);
                diff = vertical ? alloc.y - y : alloc.x - x;
            }

            if (diff > 0 && diff < sdiff) {
                sdiff = diff;
                tabIndex = i;
            }

            if (page && gtk_widget_get_visible(page)) {
                if (i > last_shown) {
                    last_shown = i;
                }
                if (first_shown == -1) {
                    first_shown = i;
                }
            }
        }

        if (qtcOneOf(tabIndex, 0, first_shown)) {
            firstTab = true;
        } else if (qtcOneOf(tabIndex, num_children - 1, last_shown)) {
            lastTab = true;
        }
        if (qtcNoneOf(highlightedTabIndex, -1, tabIndex) && !active) {
            highlight = true;
            col = &qtcPalette.background[SHADE_2_HIGHLIGHT];
        }
    }

    if (rev) {
        bool oldLast = lastTab;

        lastTab = firstTab;
        firstTab = oldLast;
    }

    if (!mozTab && qtSettings.app != GTK_APP_JAVA) {
        if (highlightedTabIndex == -1 &&
            (highlightingEnabled || opts.windowDrag >= WM_DRAG_ALL))
            qtcTabSetup(widget);
        qtcTabUpdateRect(widget, tabIndex, x, y, width, height);
    }

/*
  gtk_style_apply_default_background(style, window, widget && !qtcWidgetNoWindow(widget),
  GTK_STATE_NORMAL, area, x, y, width, height);
*/
    /*
      TODO: This is not good, should respect 'area' as passed in. However,
      it works for the moment - if the thunderbird hack above is used.
      Needs to be fixed tho.
    */

    /* In addition to the above, doing this section only for the active mozilla
       tab seems to fix some drawing errors with firefox3...*/
    /* CPD 28/02/2008 Dont need to do any of this for firefox 3beta4 */
    if (!mozTab) {
        clipArea = qtcRect(x, y, width, height);
        area = &clipArea;
    }

    glowMo = glowMo && highlight;
    drawOuterGlow = drawOuterGlow && highlight;

    switch (gapSide) {
    case GTK_POS_TOP: {
        /* tabs are on bottom !!! */
        auto round = (active || (firstTab && lastTab) ||
                      opts.tabMouseOver == TAB_MO_GLOW ||
                      opts.roundAllTabs ? ROUNDED_BOTTOM :
                      firstTab ? ROUNDED_BOTTOMLEFT : lastTab ?
                      ROUNDED_BOTTOMRIGHT : ROUNDED_NONE);
        cairo_save(cr);
        qtcClipPath(cr, x, y - 4, width, height + 4, WIDGET_TAB_BOT,
                    RADIUS_EXTERNAL, round);
        fillTab(cr, style, widget, area, state, col,
                x + mod + sizeAdjust, y, width - (2 * mod + sizeAdjust),
                height - 1, true, WIDGET_TAB_BOT, notebook != NULL);
        cairo_restore(cr);
        drawBorder(cr, style, state, area, x + sizeAdjust, y - 4,
                   width - 2 * sizeAdjust, height + 4,
                   glowMo ? qtcPalette.mouseover : qtcPalette.background,
                   round, borderProfile, WIDGET_TAB_BOT, 0);
        if (drawOuterGlow) {
            if (area) {
                area->height++;
            }
            drawGlow(cr, area, x, y - 4, width, height + 5,
                     round, WIDGET_OTHER);
        }

        if (notebook && opts.highlightTab && active) {
            qtcCairoHLine(cr, x + 1, y + height - 3, width - 2, selCol1, 0.5);
            qtcCairoHLine(cr, x + highlightOffset, y + height - 2,
                          width - 2 * highlightOffset, selCol1);

            clipArea.y = y + height - highlightBorder;
            clipArea.height = highlightBorder;
            drawBorder(cr, style, state, &clipArea, x, y,
                       width, height, qtcPalette.highlight, ROUNDED_BOTTOM,
                       BORDER_FLAT, WIDGET_OTHER, 0, 3);
        }

        if (opts.colorSelTab && notebook && active)
            colorTab(cr, x + mod + sizeAdjust, y,
                     width - (2 * mod + sizeAdjust), height - 1, round,
                     WIDGET_TAB_BOT, true);

        if (notebook && opts.coloredMouseOver && highlight &&
            opts.tabMouseOver != TAB_MO_GLOW)
            drawHighlight(cr, x + (firstTab ? moOffset : 1),
                          y + (opts.tabMouseOver == TAB_MO_TOP ?
                               height - 2 : -1),
                          width - (firstTab || lastTab ? moOffset : 1), 2,
                          NULL, true, opts.tabMouseOver != TAB_MO_TOP);
        break;
    }
    case GTK_POS_BOTTOM: {
        /* tabs are on top !!! */
        auto round = (active || (firstTab && lastTab) ||
                      opts.tabMouseOver == TAB_MO_GLOW ||
                      opts.roundAllTabs ? ROUNDED_TOP :
                      firstTab ? ROUNDED_TOPLEFT : lastTab ?
                      ROUNDED_TOPRIGHT : ROUNDED_NONE);
        cairo_save(cr);
        qtcClipPath(cr, x + mod + sizeAdjust, y,
                    width - 2 * (mod + (mozTab ? 2 * sizeAdjust : sizeAdjust)),
                    height + 5, WIDGET_TAB_TOP, RADIUS_EXTERNAL, round);
        fillTab(cr, style, widget, area, state, col,
                x + mod + sizeAdjust, y + 1,
                width - 2 * (mod + (mozTab ? 2 * sizeAdjust : sizeAdjust)),
                height - 1, true, WIDGET_TAB_TOP,notebook != NULL);
        cairo_restore(cr);
        drawBorder(cr, style, state, area, x + sizeAdjust, y,
                   width - 2 * (mozTab ? 2 : 1) * sizeAdjust, height + 4,
                   glowMo ? qtcPalette.mouseover : qtcPalette.background,
                   round, borderProfile, WIDGET_TAB_TOP, 0);
        if (drawOuterGlow) {
            if (area) {
                area->y--;
                area->height += 2;
            }
            drawGlow(cr, area, x, y - 1, width, height + 5,
                     round, WIDGET_OTHER);
        }

        if (notebook && opts.highlightTab && active) {
            qtcCairoHLine(cr, x + 1, y + 2, width - 2, selCol1, 0.5);
            qtcCairoHLine(cr, x + highlightOffset, y + 1,
                          width - 2 * highlightOffset, selCol1);

            clipArea.y = y;
            clipArea.height = highlightBorder;
            drawBorder(cr, style, state, &clipArea, x, y, width, height,
                       qtcPalette.highlight, ROUNDED_TOP, BORDER_FLAT,
                       WIDGET_OTHER, 0, 3);
        }

        if (opts.colorSelTab && notebook && active)
            colorTab(cr, x + mod + sizeAdjust, y + 1,
                     width - 2 * (mod + (mozTab ? 2 * sizeAdjust : sizeAdjust)),
                     height - 1, round, WIDGET_TAB_TOP, true);

        if (notebook && opts.coloredMouseOver && highlight &&
            opts.tabMouseOver != TAB_MO_GLOW) {
            drawHighlight(cr, x + (firstTab ? moOffset : 1),
                          y + (opts.tabMouseOver == TAB_MO_TOP ?
                               0 : height - 1),
                          width - (firstTab || lastTab ? moOffset : 1),
                          2, NULL, true, opts.tabMouseOver == TAB_MO_TOP);
        }
        break;
    }
    case GTK_POS_LEFT: {
        /* tabs are on right !!! */
        auto round = (active || (firstTab && lastTab) ||
                      opts.tabMouseOver == TAB_MO_GLOW ||
                      opts.roundAllTabs ? ROUNDED_RIGHT :
                      firstTab ? ROUNDED_TOPRIGHT : lastTab ?
                      ROUNDED_BOTTOMRIGHT : ROUNDED_NONE);
        cairo_save(cr);
        qtcClipPath(cr, x - 4, y, width + 4, height, WIDGET_TAB_BOT,
                    RADIUS_EXTERNAL, round);
        fillTab(cr, style, widget, area, state, col, x,
                y + mod + sizeAdjust, width - 1,
                height - 2 * (mod + sizeAdjust), false,
                WIDGET_TAB_BOT, notebook != NULL);
        cairo_restore(cr);
        drawBorder(cr, style, state, area, x - 4, y + sizeAdjust,
                   width + 4, height - 2 * sizeAdjust,
                   glowMo ? qtcPalette.mouseover : qtcPalette.background,
                   round, borderProfile, WIDGET_TAB_BOT, 0);
        if (drawOuterGlow) {
            if (area) {
                area->width++;
            }
            drawGlow(cr, area, x - 4, y, width + 5, height,
                     round, WIDGET_OTHER);
        }

        if (notebook && opts.highlightTab && active) {
            qtcCairoVLine(cr, x + width - 3, y + 1, height - 2, selCol1, 0.5);
            qtcCairoVLine(cr, x + width - 2, y + highlightOffset,
                          height - 2 * highlightOffset, selCol1);

            clipArea.x = x + width - highlightBorder;
            clipArea.width = highlightBorder;
            drawBorder(cr, style, state, &clipArea, x, y,
                       width, height, qtcPalette.highlight, ROUNDED_RIGHT,
                       BORDER_FLAT, WIDGET_OTHER, 0, 3);
        }

        if (opts.colorSelTab && notebook && active)
            colorTab(cr, x, y + mod + sizeAdjust, width - 1,
                     height - 2 * (mod + sizeAdjust), round,
                     WIDGET_TAB_BOT, false);

        if (notebook && opts.coloredMouseOver && highlight &&
            opts.tabMouseOver != TAB_MO_GLOW)
            drawHighlight(cr, x + (opts.tabMouseOver == TAB_MO_TOP ?
                                   width - 2 : -1),
                          y + (firstTab ? moOffset : 1), 2,
                          height - (firstTab || lastTab ? moOffset : 1),
                          NULL, false, opts.tabMouseOver != TAB_MO_TOP);
        break;
    }
    case GTK_POS_RIGHT: {
        /* tabs are on left !!! */
        auto round = (active || (firstTab && lastTab) ||
                      opts.tabMouseOver == TAB_MO_GLOW ||
                      opts.roundAllTabs ? ROUNDED_LEFT :
                      firstTab ? ROUNDED_TOPLEFT : lastTab ?
                      ROUNDED_BOTTOMLEFT : ROUNDED_NONE);
        cairo_save(cr);
        qtcClipPath(cr, x, y, width + 4, height, WIDGET_TAB_TOP,
                    RADIUS_EXTERNAL, round);
        fillTab(cr, style, widget, area, state, col, x + 1,
                y + mod + sizeAdjust, width - 1,
                height - 2 * (mod + sizeAdjust), false, WIDGET_TAB_TOP,
                notebook != NULL);
        cairo_restore(cr);
        drawBorder(cr, style, state, area, x, y + sizeAdjust,
                   width + 4, height - 2 * sizeAdjust,
                   glowMo ? qtcPalette.mouseover : qtcPalette.background,
                   round, borderProfile, WIDGET_TAB_TOP, 0);
        if (drawOuterGlow) {
            if (area) {
                area->x--;
                area->width += 2;
            }
            drawGlow(cr, area, x - 1, y, width + 5, height,
                     round, WIDGET_OTHER);
        }
        if (notebook && opts.highlightTab && active) {
            qtcCairoVLine(cr, x + 2, y + 1, height - 2, selCol1, 0.5);
            qtcCairoVLine(cr, x + 1, y + highlightOffset,
                          height - 2 * highlightOffset, selCol1);

            clipArea.x = x;
            clipArea.width = highlightBorder;
            drawBorder(cr, style, state, &clipArea, x, y,
                       width, height, qtcPalette.highlight, ROUNDED_LEFT,
                       BORDER_FLAT, WIDGET_OTHER, 0, 3);
        }

        if (opts.colorSelTab && notebook && active)
            colorTab(cr, x + 1, y + mod + sizeAdjust, width - 1,
                     height - 2 * (mod + sizeAdjust), round,
                     WIDGET_TAB_TOP, false);

        if (notebook && opts.coloredMouseOver && highlight &&
            opts.tabMouseOver != TAB_MO_GLOW)
            drawHighlight(cr, x + (opts.tabMouseOver == TAB_MO_TOP ?
                                   0 : width - 1),
                          y + (firstTab ? moOffset : 1), 2,
                          height - (firstTab || lastTab ? moOffset : 1),
                          NULL, false, opts.tabMouseOver == TAB_MO_TOP);
        break;
    }
    }
}

void
drawToolbarBorders(cairo_t *cr, GtkStateType state, int x, int y, int width,
                   int height, bool isActiveWindowMenubar, const char *detail)
{
    bool top = false;
    bool bottom = false;
    bool left = false;
    bool right = false;
    bool all = qtcOneOf(opts.toolbarBorders, TB_LIGHT_ALL, TB_DARK_ALL);
    int border = qtcOneOf(opts.toolbarBorders, TB_DARK, TB_DARK_ALL) ? 3 : 4;
    const GdkColor *cols = (isActiveWindowMenubar &&
                            (state != GTK_STATE_INSENSITIVE ||
                             opts.shadeMenubars != SHADE_NONE) ?
                            menuColors(isActiveWindowMenubar) :
                            qtcPalette.background);
    if (DETAIL("menubar")) {
        if (all) {
            top = bottom = left = right = true;
        } else {
            bottom = true;
        }
    } else if (strcmp(detail,"toolbar") == 0) {
        if (all) {
            if (width < height) {
                left = right = bottom = true;
            } else {
                top = bottom = right = true;
            }
        } else {
            if (width < height) {
                left = right = true;
            } else {
                top = bottom = true;
            }
        }
    } else if (strcmp(detail,"dockitem_bin") == 0 ||
               strcmp(detail, "handlebox_bin") == 0) {
        /* CPD: bit risky - what if only 1 item ??? */
        if (all) {
            if (width < height) {
                left = right = bottom = true;
            } else {
                top = bottom = right = true;
            }
        } else {
            if (width < height) {
                left = right = true;
            } else {
                top = bottom = true;
            }
        }
    } else {
        /* handle */
        if (all) {
            if (width < height) {
                /* on horiz toolbar */
                top = bottom = left = true;
            } else {
                left = right = top = true;
            }
        } else {
            if (width < height) {
                /* on horiz toolbar */
                top = bottom = true;
            } else {
                left = right = true;
            }
        }
    }
    if (top) {
        qtcCairoHLine(cr, x, y, width, cols);
    }
    if (left) {
        qtcCairoVLine(cr, x, y, height, cols);
    }
    if (bottom) {
        qtcCairoHLine(cr, x, y + height - 1, width, &cols[border]);
    }
    if (right) {
        qtcCairoVLine(cr, x + width - 1, y, height, &cols[border]);
    }
}

void
drawListViewHeader(cairo_t *cr, GtkStateType state, const GdkColor *btnColors,
                   int bgnd, const QtcRect *area, int x, int y,
                   int width, int height)
{
    drawBevelGradient(cr, area, x, y, width, height, &btnColors[bgnd],
                      true, state == GTK_STATE_ACTIVE || bgnd == 2 || bgnd == 3,
                      opts.lvAppearance, WIDGET_LISTVIEW_HEADER);

    if (opts.lvAppearance == APPEARANCE_RAISED) {
        qtcCairoHLine(cr, x, y + height - 2, width, &qtcPalette.background[4]);
    }
    qtcCairoHLine(cr, x, y + height - 1, width,
                  &qtcPalette.background[QTC_STD_BORDER]);

    if (state == GTK_STATE_PRELIGHT && opts.coloredMouseOver) {
        drawHighlight(cr, x, y + height - 2, width, 2, area, true, true);
    }

#if GTK_CHECK_VERSION(2, 90, 0) /* Gtk3:TODO !!! */
    drawFadedLine(cr, x + width - 2, y + 4, 1, height - 8,
                  &btnColors[QTC_STD_BORDER], area, NULL, true, true, false);
    drawFadedLine(cr, x + width - 1, y + 4, 1, height - 8, &btnColors[0],
                  area, NULL, true, true, false);
#else
    if (x > 3 && height > 10) {
        drawFadedLine(cr, x, y + 4, 1, height - 8, &btnColors[QTC_STD_BORDER],
                      area, NULL, true, true, false);
        drawFadedLine(cr, x + 1, y + 4, 1, height - 8, &btnColors[0],
                      area, NULL, true, true, false);
    }
#endif
}

void
drawDefBtnIndicator(cairo_t *cr, GtkStateType state, const GdkColor *btnColors,
                    int bgnd, bool sunken, const QtcRect *area, int x, int y,
                    int width, int height)
{
    if (opts.defBtnIndicator == IND_CORNER) {
        int offset = sunken ? 5 : 4;
        int etchOffset = opts.buttonEffect != EFFECT_NONE ? 1 : 0;
        const GdkColor *cols = (qtcPalette.focus ? qtcPalette.focus :
                                qtcPalette.highlight);
        const GdkColor *col = &cols[state == GTK_STATE_ACTIVE ? 0 : 4];

        cairo_new_path(cr);
        qtcCairoSetColor(cr, col);
        cairo_move_to(cr, x + offset + etchOffset, y + offset + etchOffset);
        cairo_line_to(cr, x + offset + 6 + etchOffset, y + offset + etchOffset);
        cairo_line_to(cr, x + offset + etchOffset, y + offset + 6 + etchOffset);
        cairo_fill(cr);
    } else if (opts.defBtnIndicator == IND_COLORED && COLORED_BORDER_SIZE > 2) {
        // offset needed because of etch
        int o = (COLORED_BORDER_SIZE +
                 (opts.buttonEffect != EFFECT_NONE ? 1 : 0));
        drawBevelGradient(cr, area, x + o, y + o, width - 2 * o,
                          height - 2 * o, &btnColors[bgnd], true,
                          state == GTK_STATE_ACTIVE, opts.appearance,
                          WIDGET_STD_BUTTON);
    }
}

static GdkPixbuf*
scaleOrRef(GdkPixbuf *src, int width, int height)
{
    if (gdk_pixbuf_get_width(src) == width &&
        gdk_pixbuf_get_height(src) == height) {
        return (GdkPixbuf*)g_object_ref(G_OBJECT(src));
    } else {
        return gdk_pixbuf_scale_simple(src, width, height, GDK_INTERP_BILINEAR);
    }
}

static GdkPixbuf*
setTransparency(const GdkPixbuf *pixbuf, double alpha_percent)
{
    QTC_RET_IF_FAIL(pixbuf != NULL, NULL);
    QTC_RET_IF_FAIL(GDK_IS_PIXBUF(pixbuf), NULL);
    /* Returns a copy of pixbuf with it's non-completely-transparent pixels to
       have an alpha level "alpha_percent" of their original value. */
    GdkPixbuf *target = gdk_pixbuf_add_alpha (pixbuf, false, 0, 0, 0);
    if (alpha_percent == 1.0) {
        return target;
    } else {
        unsigned width = gdk_pixbuf_get_width(target);
        unsigned height = gdk_pixbuf_get_height(target);
        unsigned rowstride = gdk_pixbuf_get_rowstride(target);
        unsigned char *data = gdk_pixbuf_get_pixels(target);
        for (unsigned y = 0;y < height;y++) {
            for (unsigned x = 0;x < width;x++) {
                data[y * rowstride + x * 4 + 3] *= alpha_percent;
            }
        }
    }
    return target;
}

GdkPixbuf*
renderIcon(GtkStyle *style, const GtkIconSource *source, GtkStateType state,
           GtkIconSize size, GtkWidget *widget)
{
    int width = 1;
    int height = 1;
    GdkPixbuf *scaled;
    GdkPixbuf *stated;
    GdkPixbuf *base_pixbuf;
    GdkScreen *screen;
    GtkSettings *settings;
    bool scaleMozilla = (opts.mapKdeIcons && isMozilla() &&
                         size == GTK_ICON_SIZE_DIALOG);

    /* Oddly, style can be NULL in this function, because GtkIconSet can be
     * used without a style and if so it uses this function. */
    base_pixbuf = gtk_icon_source_get_pixbuf(source);

    QTC_RET_IF_FAIL(base_pixbuf != NULL, NULL);

    if (widget && gtk_widget_has_screen(widget)) {
        screen = gtk_widget_get_screen(widget);
        settings = screen ? gtk_settings_get_for_screen(screen) : NULL;
    }
#if GTK_CHECK_VERSION(2, 90, 0)
    else if (style->visual) {
        screen = gdk_visual_get_screen(style->visual);
        settings = screen ? gtk_settings_get_for_screen(screen) : NULL;
    }
#else
    else if(style->colormap) {
        screen = gdk_colormap_get_screen(style->colormap);
        settings = screen ? gtk_settings_get_for_screen(screen) : NULL;
    }
#endif
    else {
        settings = gtk_settings_get_default();
        GTK_NOTE(MULTIHEAD, g_warning("Using the default screen for "
                                      "gtk_default_render_icon()"));
    }

    if (scaleMozilla) {
        width = height = 48;
    } else if (size != (GtkIconSize)-1 &&
               !gtk_icon_size_lookup_for_settings(settings, size,
                                                  &width, &height)) {
        g_warning(G_STRLOC ": invalid icon size '%d'", size);
        return NULL;
    }

    /* If the size was wildcarded, and we're allowed to scale, then scale;
     * otherwise, leave it alone. */
    if (scaleMozilla || (size != (GtkIconSize)-1 &&
                         gtk_icon_source_get_size_wildcarded(source))) {
        scaled = scaleOrRef(base_pixbuf, width, height);
    } else {
        scaled = (GdkPixbuf*)g_object_ref(G_OBJECT(base_pixbuf));
    }

    /* If the state was wildcarded, then generate a state. */
    if (gtk_icon_source_get_state_wildcarded(source)) {
        if (state == GTK_STATE_INSENSITIVE) {
            stated = setTransparency(scaled, 0.5);
            gdk_pixbuf_saturate_and_pixelate(stated, stated, 0.0, false);
            g_object_unref(scaled);
        }
#if 0 /* KDE does not highlight icons */
        else if (state == GTK_STATE_PRELIGHT) {
            stated = gdk_pixbuf_copy(scaled);
            gdk_pixbuf_saturate_and_pixelate(scaled, stated, 1.2, false);
            g_object_unref(scaled);
        }
#endif
        else {
            stated = scaled;
        }
    } else {
        stated = scaled;
    }
    return stated;
}
