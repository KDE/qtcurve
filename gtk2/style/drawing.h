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

#ifndef __QTC_DRAWING_H__
#define __QTC_DRAWING_H__

#include <common/common.h>
#include <qtcurve-cairo/draw.h>

#define CAIRO_GRAD_END 1.0

QTC_BEGIN_DECLS

void drawBgnd(cairo_t *cr, const GdkColor *col, GtkWidget *widget,
              const QtcRect *area, int x, int y, int width, int height);
void drawAreaModColor(cairo_t *cr, const QtcRect *area, const GdkColor *orig,
                      double mod, int x, int y, int width, int height);
QTC_ALWAYS_INLINE static inline void
drawAreaMod(cairo_t *cr, GtkStyle *style, GtkStateType state,
            const QtcRect *area, double mod, int x, int y,
            int width, int height)
{
    drawAreaModColor(cr, area, &style->bg[state], mod, x, y, width, height);
}

void drawBevelGradient(cairo_t *cr, const QtcRect *area, int x, int y,
                       int width, int height, const GdkColor *base, bool horiz,
                       bool sel, EAppearance bevApp, EWidget w, double alpha);
#define drawBevelGradient(cr, area, x, y, width, height, base, horiz, sel, \
                          bevApp, w, alpha...)                          \
    drawBevelGradient(cr, area, x, y, width, height, base, horiz, sel,  \
                      bevApp, w, QTC_DEFAULT(alpha, 1))

typedef enum {
    DF_DRAW_INSIDE = 0x001,
    DF_BLEND = 0x002,
    DF_SUNKEN = 0x004,
    DF_DO_BORDER = 0x008,
    DF_VERT = 0x010,
    DF_HIDE_EFFECT = 0x020,
    DF_HAS_FOCUS = 0x040
} EDrawFlags;

void drawBorder(cairo_t *cr, GtkStyle *style, GtkStateType state,
                const QtcRect *area, int x, int y, int width, int height,
                const GdkColor *c_colors, ECornerBits round,
                EBorder borderProfile, EWidget widget, int flags,
                int borderVal);
#define drawBorder(cr, style, state, area, x, y, width, height, c_colors, \
                   round, borderProfile, widget, flags, borderVal...)   \
    drawBorder(cr, style, state, area, x, y, width, height, c_colors,   \
               round, borderProfile, widget, flags,                     \
               QTC_DEFAULT(borderVal, QTC_STD_BORDER))

void drawGlow(cairo_t *cr, const QtcRect *area, int x, int y, int w, int h,
              ECornerBits round, EWidget widget, const GdkColor *colors);
#define drawGlow(cr, area, x, y, w, h, round, widget, colors...)        \
    drawGlow(cr, area, x, y, w, h, round, widget, QTC_DEFAULT(colors, NULL))

void drawEtch(cairo_t *cr, const QtcRect *area, GtkWidget *widget, int x, int y,
              int w, int h, bool raised, ECornerBits round, EWidget wid);
void qtcClipPath(cairo_t *cr, int x, int y, int w, int h, EWidget widget,
                 ERadius rad, ECornerBits round);
void drawLightBevel(cairo_t *cr, GtkStyle *style, GtkStateType state,
                    const QtcRect *area, int x, int y, int width, int height,
                    const GdkColor *base, const GdkColor *colors,
                    ECornerBits round, EWidget widget, EBorder borderProfile,
                    int flags, GtkWidget *wid);

void drawFadedLine(cairo_t *cr, int x, int y, int width, int height,
                   const GdkColor *col, const QtcRect *area,
                   const QtcRect *gap, bool fadeStart, bool fadeEnd,
                   bool horiz, double alpha);
#define drawFadedLine(cr, x, y, width, height, col, area, gap, fadeStart, \
                      fadeEnd, horiz, alpha...)                         \
    drawFadedLine(cr, x, y, width, height, col, area, gap, fadeStart,   \
                  fadeEnd, horiz, QTC_DEFAULT(alpha, 1))

void drawHighlight(cairo_t *cr, int x, int y, int width, int height,
                   const QtcRect *area, bool horiz, bool inc);
void setLineCol(cairo_t *cr, cairo_pattern_t *pt, const GdkColor *col);
void drawLines(cairo_t *cr, double rx, double ry, int rwidth, int rheight,
               bool horiz, int nLines, int offset, const GdkColor *cols,
               const QtcRect *area, int dark, ELine type);
void drawEntryCorners(cairo_t *cr, const QtcRect *area, int round, int x, int y,
                      int width, int height, const GdkColor *col, double a);
void drawBgndRing(cairo_t *cr, int x, int y, int size,
                  int size2, bool isWindow);
void drawBgndRings(cairo_t *cr, int x, int y, int width, int height,
                   bool isWindow);
void drawBgndImage(cairo_t *cr, int x, int y, int w, int h, bool isWindow);
void drawStripedBgnd(cairo_t *cr, int x, int y, int w, int h,
                     const GdkColor *col, double alpha);
bool drawWindowBgnd(cairo_t *cr, GtkStyle *style, const QtcRect *area,
                    GdkWindow *window, GtkWidget *widget, int x, int y,
                    int width, int height);
void drawEntryField(cairo_t *cr, GtkStyle *style, GtkStateType state,
                    GdkWindow *window, GtkWidget *widget, const QtcRect *area,
                    int x, int y, int width, int height, ECornerBits round,
                    EWidget w);
void drawProgress(cairo_t *cr, GtkStyle *style, GtkStateType state,
                  GtkWidget *widget, const QtcRect *area, int x, int y,
                  int width, int height, bool rev, bool isEntryProg);
void drawProgressGroove(cairo_t *cr, GtkStyle *style, GtkStateType state,
                        GdkWindow *window, GtkWidget *widget,
                        const QtcRect *area, int x, int y, int width,
                        int height, bool isList, bool horiz);
void drawSliderGroove(cairo_t *cr, GtkStyle *style, GtkStateType state,
                      GtkWidget *widget, const char *detail,
                      const QtcRect *area, int x, int y, int width, int height,
                      bool horiz);
void drawTriangularSlider(cairo_t *cr, GtkStyle *style, GtkStateType state,
                          const char *detail, int x, int y,
                          int width, int height);
void drawScrollbarGroove(cairo_t *cr, GtkStyle *style, GtkStateType state,
                         GtkWidget *widget, const QtcRect *area, int x, int y,
                         int width, int height, bool horiz);
void drawSelectionGradient(cairo_t *cr, const QtcRect *area, int x, int y,
                           int width, int height, ECornerBits round,
                           bool isLvSelection, double alpha,
                           const GdkColor *col, bool horiz);
void drawSelection(cairo_t *cr, GtkStyle *style, GtkStateType state,
                   const QtcRect *area, GtkWidget *widget, int x, int y,
                   int width, int height, ECornerBits round, bool isLvSelection,
                   double alphaMod, int factor);
void createRoundedMask(GtkWidget *widget, int x, int y, int width, int height,
                       double radius, bool isToolTip);
void clearRoundedMask(GtkWidget *widget, bool isToolTip);
void drawTreeViewLines(cairo_t *cr, const GdkColor *col, int x, int y, int h,
                       int depth, int levelIndent, int expanderSize,
                       GtkTreeView *treeView, GtkTreePath *path);
void fillTab(cairo_t *cr, GtkStyle *style, GtkWidget *widget,
             const QtcRect *area, GtkStateType state, const GdkColor *col,
             int x, int y, int width, int height, bool horiz,
             EWidget tab, bool grad);
void drawToolTip(cairo_t *cr, GtkWidget *widget, const QtcRect *area,
                 int x, int y, int width, int height);
void drawSplitter(cairo_t *cr, GtkStateType state, GtkStyle *style,
                  const QtcRect *area, int x, int y, int width, int height);
void drawSidebarButton(cairo_t *cr, GtkStateType state, GtkStyle *style,
                       const QtcRect *area, int x, int y,
                       int width, int height);
void drawMenuItem(cairo_t *cr, GtkStateType state, GtkStyle *style,
                  GtkWidget *widget, const QtcRect *area, int x, int y,
                  int width, int height);
void drawMenu(cairo_t *cr, GtkWidget *widget, const QtcRect *area,
              int x, int y, int width, int height);
void drawBoxGap(cairo_t *cr, GtkStyle *style, GtkShadowType shadow,
                GtkStateType state, GtkWidget *widget, const QtcRect *area,
                int x, int y, int width, int height, GtkPositionType gap_side,
                int gapX, int gapWidth, EBorder borderProfile, bool isTab);
void drawBoxGapFixes(cairo_t *cr, GtkWidget *widget, int x, int y,
                     int width, int height, GtkPositionType gapSide,
                     int gapX, int gapWidth);
void drawShadowGap(cairo_t *cr, GtkStyle *style, GtkShadowType shadow,
                   GtkStateType state, GtkWidget *widget, const QtcRect *area,
                   int x, int y, int width, int height,
                   GtkPositionType gapSide, int gapX, int gapWidth);
void drawCheckBox(cairo_t *cr, GtkStateType state, GtkShadowType shadow,
                  GtkStyle *style, GtkWidget *widget, const char *detail,
                  const QtcRect *area, int x, int y, int width, int height);
void drawTab(cairo_t *cr, GtkStateType state, GtkStyle *style,
             GtkWidget *widget, QtcRect *area, int x, int y,
             int width, int height, GtkPositionType gapSide);
void drawRadioButton(cairo_t *cr, GtkStateType state, GtkShadowType shadow,
                     GtkStyle *style, GtkWidget *widget, const char *detail,
                     const QtcRect *area, int x, int y, int width, int height);
void drawToolbarBorders(cairo_t *cr, GtkStateType state, int x, int y,
                        int width, int height, bool isActiveWindowMenubar,
                        const char *detail);
void drawListViewHeader(cairo_t *cr, GtkStateType state,
                        const GdkColor *btnColors, int bgnd,
                        const QtcRect *area, int x, int y,
                        int width, int height);
void drawDefBtnIndicator(cairo_t *cr, GtkStateType state,
                         const GdkColor *btnColors, int bgnd, bool sunken,
                         const QtcRect *area, int x, int y,
                         int width, int height);
GdkPixbuf *renderIcon(GtkStyle *style, const GtkIconSource *source,
                      GtkStateType state, GtkIconSize size, GtkWidget *widget);

QTC_END_DECLS

#endif
