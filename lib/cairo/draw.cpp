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

#include "draw.h"
#include "utils_p.h"
#include <pango/pangocairo.h>

namespace QtCurve {
namespace Cairo {

QTC_EXPORT void
hLine(cairo_t *cr, int x, int y, int w, const GdkColor *col, double a)
{
    Saver saver(cr);
    cairo_new_path(cr);
    setColor(cr, col, a);
    cairo_move_to(cr, x, y + 0.5);
    cairo_line_to(cr, x + w, y + 0.5);
    cairo_stroke(cr);
}

QTC_EXPORT void
vLine(cairo_t *cr, int x, int y, int h, const GdkColor *col, double a)
{
    Saver saver(cr);
    cairo_new_path(cr);
    setColor(cr, col, a);
    cairo_move_to(cr, x + 0.5, y);
    cairo_line_to(cr, x + 0.5, y + h);
    cairo_stroke(cr);
}

QTC_EXPORT void
polygon(cairo_t *cr, const GdkColor *col, const QtcRect *area,
        const GdkPoint *points, int npoints, bool fill)
{
    Saver saver(cr);
    cairo_set_line_width(cr, 1);
    clipRect(cr, area);
    setColor(cr, col);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    pathPoints(cr, points, npoints);
    cairo_close_path(cr);
    cairo_stroke_preserve(cr);
    if (fill) {
        cairo_fill(cr);
    }
}

QTC_EXPORT void
rect(cairo_t *cr, const QtcRect *area, int x, int y, int width, int height,
     const GdkColor *col, double alpha)
{
    Saver saver(cr);
    clipRect(cr, area);
    cairo_rectangle(cr, x, y, width, height);
    setColor(cr, col, alpha);
    cairo_fill(cr);
}

QTC_EXPORT void
fadedLine(cairo_t *cr, int x, int y, int width, int height,
          const QtcRect *area, const QtcRect *gap, bool fadeStart,
          bool fadeEnd, double fadeSize, bool horiz,
          const GdkColor *col, double alpha)
{
    double rx = x + 0.5;
    double ry = y + 0.5;
    cairo_pattern_t *pt =
        cairo_pattern_create_linear(rx, ry, horiz ? rx + width - 1 : rx + 1,
                                    horiz ? ry + 1 : ry + height - 1);

    Saver saver(cr);
    if (gap) {
        QtcRect r = {x, y, width, height};
        cairo_region_t *region =
            cairo_region_create_rectangle(area ? area : &r);
        cairo_region_xor_rectangle(region, gap);
        clipRegion(cr, region);
        cairo_region_destroy(region);
    } else {
        clipRect(cr, area);
    }
    patternAddColorStop(pt, 0, col, fadeStart ? 0.0 : alpha);
    patternAddColorStop(pt, fadeSize, col, alpha);
    patternAddColorStop(pt, 1 - fadeSize, col, alpha);
    patternAddColorStop(pt, 1, col, fadeEnd ? 0.0 : alpha);
    cairo_set_source(cr, pt);
    if (horiz) {
        cairo_move_to(cr, x, ry);
        cairo_line_to(cr, x + width - 1, ry);
    } else {
        cairo_move_to(cr, rx, y);
        cairo_line_to(cr, rx, y + height - 1);
    }
    cairo_stroke(cr);
    cairo_pattern_destroy(pt);
}

QTC_EXPORT void
stripes(cairo_t *cr, int x, int y, int w, int h,
        bool horizontal, int stripeWidth)
{
    int endx = horizontal ? stripeWidth : 0;
    int endy = horizontal ? 0 : stripeWidth;
    cairo_pattern_t *pat =
        cairo_pattern_create_linear(x, y, x + endx, y + endy);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 1.0, 1.0, 1.0, 0.0);
    cairo_pattern_add_color_stop_rgba(pat, 1, 1.0, 1.0, 1.0, 0.15);
    cairo_pattern_set_extend(pat, CAIRO_EXTEND_REFLECT);

    Saver saver(cr);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);

    cairo_pattern_destroy(pat);
}

QTC_EXPORT void
dot(cairo_t *cr, int x, int y, int w, int h, const GdkColor *col)
{
    double dx = x + (w - 5) / 2;
    double dy = y + (h - 5) / 2;
    cairo_pattern_t *p1 = cairo_pattern_create_linear(dx, dy, dx + 4, dy + 4);
    cairo_pattern_t *p2 = cairo_pattern_create_linear(dx + 2, dy + 2,
                                                      dx + 4, dx + 4);

    patternAddColorStop(p1, 0, col, 1);
    patternAddColorStop(p1, 1, col, 0.4);
    cairo_pattern_add_color_stop_rgba(p2, 1, 1, 1, 1, 0.9);
    cairo_pattern_add_color_stop_rgba(p2, 0, 1, 1, 1, 0.7);

    Saver saver(cr);
    cairo_new_path(cr);
    cairo_arc(cr, dx + 2.5, dy + 2.5, 2.5, 0, 2 * M_PI);
    cairo_clip(cr);
    cairo_set_source(cr, p1);
    cairo_rectangle(cr, dx, dy, 5, 5);
    cairo_fill(cr);

    cairo_new_path(cr);
    cairo_arc(cr, dx + 3, dy + 3, 2, 0, 2 * M_PI);
    cairo_clip(cr);
    cairo_set_source(cr, p2);
    cairo_rectangle(cr, dx + 1, dy + 1, 4, 4);
    cairo_fill(cr);

    cairo_pattern_destroy(p1);
    cairo_pattern_destroy(p2);
}

QTC_EXPORT void
dots(cairo_t *cr, int rx, int ry, int rwidth, int rheight, bool horiz,
     int nLines, int offset, const QtcRect *area, int startOffset,
     const GdkColor *col1, const GdkColor *col2)
{
    int space = nLines * 2 + nLines - 1;
    int x = horiz ? rx : rx + (rwidth - space) / 2;
    int y = horiz ? ry + (rheight - space) / 2 : ry;
    int numDots = ((horiz ? rwidth : rheight) - 2 * offset) / 3 + 1;

    Saver saver(cr);
    clipRect(cr, area);
    if (horiz) {
        if (startOffset && y + startOffset > 0) {
            y += startOffset;
        }
        cairo_new_path(cr);
        setColor(cr, col1);
        for (int i = 0;i < space;i += 3) {
            for (int j = 0;j < numDots;j++) {
                cairo_rectangle(cr, x + offset + 3 * j, y + i, 1, 1);
            }
        }
        cairo_fill(cr);

        cairo_new_path(cr);
        setColor(cr, col2);
        for (int i = 1;i < space;i += 3) {
            for (int j = 0;j < numDots;j++) {
                cairo_rectangle(cr, x + offset + 1 + 3 * j, y + i, 1, 1);
            }
        }
        cairo_fill(cr);
    } else {
        if (startOffset && x + startOffset > 0) {
            x += startOffset;
        }
        cairo_new_path(cr);
        setColor(cr, col1);
        for (int i = 0;i < space;i += 3) {
            for (int j = 0;j < numDots;j++) {
                cairo_rectangle(cr, x + i, y + offset + 3 * j, 1, 1);
            }
        }
        cairo_fill(cr);

        cairo_new_path(cr);
        setColor(cr, col2);
        for (int i = 1;i < space;i += 3) {
            for(int j = 0;j < numDots;j++) {
                cairo_rectangle(cr, x + i, y + offset + 1 + 3 * j, 1, 1);
            }
        }
        cairo_fill(cr);
    }
}

static void
ge_transform_for_layout(cairo_t *cr, PangoLayout *layout, int x, int y)
{
    const PangoMatrix *matrix =
        pango_context_get_matrix(pango_layout_get_context(layout));
    if (matrix) {
        cairo_matrix_t cairo_matrix;
        PangoRectangle _rect;

        cairo_matrix_init(&cairo_matrix, matrix->xx, matrix->yx,
                          matrix->xy, matrix->yy, matrix->x0, matrix->y0);
        pango_layout_get_extents(layout, nullptr, &_rect);
        pango_matrix_transform_rectangle(matrix, &_rect);
        pango_extents_to_pixels(&_rect, nullptr);

        cairo_matrix.x0 += x - _rect.x;
        cairo_matrix.y0 += y - _rect.y;

        cairo_set_matrix(cr, &cairo_matrix);
    } else {
        cairo_translate(cr, x, y);
    }
}

QTC_EXPORT void
layout(cairo_t *cr, const QtcRect *area, int x, int y, PangoLayout *_layout,
       const GdkColor *col)
{
    Saver saver(cr);
    clipRect(cr, area);
    cairo_set_line_width(cr, 1);
    setColor(cr, col);
    ge_transform_for_layout(cr, _layout, x, y);
    pango_cairo_show_layout(cr, _layout);
}

QTC_EXPORT void
arrow(cairo_t *cr, const GdkColor *col, const QtcRect *area,
      ArrowType arrow_type, int x, int y, bool small, bool fill, bool varrow)
{
    if (small) {
        switch (arrow_type) {
        case ArrowType::Up: {
            const GdkPoint a[] = {{x + 2, y}, {x, y - 2}, {x - 2, y},
                                  {x - 2, y + 1}, {x, y - 1}, {x + 2, y + 1}};
            polygon(cr, col, area, a, varrow ? 6 : 3, fill);
            break;
        }
        case ArrowType::Down: {
            const GdkPoint a[] = {{x + 2, y}, {x, y + 2}, {x - 2, y},
                                  {x - 2, y - 1}, {x, y + 1}, {x + 2, y - 1}};
            polygon(cr, col, area, a, varrow ? 6 : 3, fill);
            break;
        }
        case ArrowType::Right: {
            const GdkPoint a[] = {{x, y - 2}, {x + 2, y}, {x, y + 2},
                                  {x - 1, y + 2}, {x + 1, y}, {x - 1, y - 2}};
            polygon(cr, col, area, a, varrow ? 6 : 3, fill);
            break;
        }
        case ArrowType::Left: {
            const GdkPoint a[] = {{x, y - 2}, {x - 2, y}, {x, y + 2},
                                  {x + 1, y + 2}, {x - 1, y}, {x + 1, y - 2}};
            polygon(cr, col, area, a, varrow ? 6 : 3, fill);
            break;
        }
        default:
            break;
        }
    } else {
        /* Large arrows... */
        switch (arrow_type) {
        case ArrowType::Up: {
            const GdkPoint a[] = {{x + 3, y + 1}, {x, y - 2}, {x - 3, y + 1},
                                  {x - 3, y + 2}, {x - 2, y + 2}, {x, y},
                                  {x + 2, y + 2}, {x + 3, y + 2}};
            polygon(cr, col, area, a, varrow ? 8 : 3, fill);
            break;
        }
        case ArrowType::Down: {
            const GdkPoint a[] = {{x + 3, y - 1}, {x, y + 2}, {x - 3, y - 1},
                                  {x - 3, y - 2}, {x - 2, y - 2}, {x, y},
                                  {x + 2, y - 2}, {x + 3,y - 2}};
            polygon(cr, col, area, a, varrow ? 8 : 3, fill);
            break;
        }
        case ArrowType::Right: {
            const GdkPoint a[] = {{x - 1, y + 3}, {x + 2, y}, {x - 1, y - 3},
                                  {x - 2, y - 3}, {x - 2, y - 2}, {x, y},
                                  {x - 2, y + 2}, {x - 2, y + 3}};
            polygon(cr, col, area, a, varrow ? 8 : 3, fill);
            break;
        }
        case ArrowType::Left: {
            const GdkPoint a[] = {{x + 1, y - 3}, {x - 2, y}, {x + 1, y + 3},
                                  {x + 2, y + 3}, {x + 2, y + 2}, {x, y},
                                  {x + 2, y - 2}, {x + 2, y - 3}};
            polygon(cr, col, area, a, varrow ? 8 : 3, fill);
            break;
        }
        default:
            break;
        }
    }
}

}
}
