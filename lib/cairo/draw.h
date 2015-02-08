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

#ifndef __QTC_CAIRO_DRAW_H__
#define __QTC_CAIRO_DRAW_H__

#include "utils.h"
#include <pango/pango.h>

#ifdef QTC_UTILS_GTK
#include <gtk/gtk.h>
#endif

namespace QtCurve {
enum class ArrowType {
    Up,
    Down,
    Left,
    Right,
    NONE,
};

namespace Cairo {

void hLine(cairo_t *cr, int x, int y, int w, const _GdkColor *col, double a=1);
void vLine(cairo_t *cr, int x, int y, int w, const _GdkColor *col, double a=1);
void polygon(cairo_t *cr, const _GdkColor *col, const QtcRect *area,
             const _GdkPoint *points, int npoints, bool fill);
void rect(cairo_t *cr, const QtcRect *area, int x, int y, int width,
          int height, const _GdkColor *col, double alpha=1);
void fadedLine(cairo_t *cr, int x, int y, int width, int height,
               const QtcRect *area, const QtcRect *gap, bool fadeStart,
               bool fadeEnd, double fadeSize, bool horiz,
               const _GdkColor *col, double alpha=1);
void stripes(cairo_t *cr, int x, int y, int w, int h,
             bool horizontal, int stripeWidth);
void dot(cairo_t *cr, int x, int y, int w, int h, const _GdkColor *col);
void dots(cairo_t *cr, int rx, int ry, int rwidth, int rheight,
          bool horiz, int nLines, int offset, const QtcRect *area,
          int startOffset, const _GdkColor *col1,
          const _GdkColor *col2);
void layout(cairo_t *cr, const QtcRect *area, int x, int y,
            PangoLayout *layout, const _GdkColor *col);
void arrow(cairo_t *cr, const _GdkColor *col,
           const QtcRect *area, ArrowType arrow_type,
           int x, int y, bool small, bool fill, bool varrow);

#ifdef QTC_UTILS_GTK
static inline ArrowType
arrowType(GtkArrowType gtype)
{
    switch (gtype) {
    case GTK_ARROW_UP:
        return ArrowType::Up;
    case GTK_ARROW_DOWN:
        return ArrowType::Down;
    case GTK_ARROW_LEFT:
        return ArrowType::Left;
    case GTK_ARROW_RIGHT:
        return ArrowType::Right;
    default:
        return ArrowType::NONE;
    }
}

static inline void
arrow(cairo_t *cr, const _GdkColor *col,
      const QtcRect *area, GtkArrowType arrow_type,
      int x, int y, bool small, bool fill, bool varrow)
{
    arrow(cr, col, area, arrowType(arrow_type), x, y, small, fill, varrow);
}

#endif

}
}

#endif
