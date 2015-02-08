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

#include "utils_p.h"
#include <qtcurve-utils/number.h>

namespace QtCurve {
namespace Rect {

QTC_EXPORT void
union_(const QtcRect *src1, const QtcRect *src2, QtcRect *dest)
{
    // Copied from gdk_rectangle_union
    int dest_x = qtcMin(src1->x, src2->x);
    int dest_y = qtcMin(src1->y, src2->y);
    dest->width = qtcMax(src1->x + src1->width,
                         src2->x + src2->width) - dest_x;
    dest->height = qtcMax(src1->y + src1->height,
                          src2->y + src2->height) - dest_y;
    dest->x = dest_x;
    dest->y = dest_y;
}

QTC_EXPORT bool
intersect(const QtcRect *src1, const QtcRect *src2, QtcRect *dest)
{
    // Copied from gdk_rectangle_intersect
    int dest_x = qtcMax(src1->x, src2->x);
    int dest_y = qtcMax(src1->y, src2->y);
    int dest_x2 = qtcMin(src1->x + src1->width, src2->x + src2->width);
    int dest_y2 = qtcMin(src1->y + src1->height, src2->y + src2->height);

    if (dest_x2 > dest_x && dest_y2 > dest_y) {
        if (dest) {
            dest->x = dest_x;
            dest->y = dest_y;
            dest->width = dest_x2 - dest_x;
            dest->height = dest_y2 - dest_y;
        }
        return true;
    } else if (dest) {
        dest->width = 0;
        dest->height = 0;
    }
    return false;
}

}

namespace Cairo {

QTC_EXPORT void
pathPoints(cairo_t *cr, const GdkPoint *pts, int count)
{
    // The (0.5, 0.5) offset here is for moving the path to the center of the
    // pixel.
    cairo_move_to(cr, pts[0].x + 0.5, pts[0].y + 0.5);
    for (int i = 1;i < count;i++) {
        cairo_line_to(cr, pts[i].x + 0.5, pts[i].y + 0.5);
    }
}

static void
pathRegion(cairo_t *cr, const cairo_region_t *region)
{
    // Copied from gdk_cairo_region.
    int n_boxes = cairo_region_num_rectangles(region);
    QtcRect box;
    for (int i = 0;i < n_boxes;i++) {
        cairo_region_get_rectangle(region, i, &box);
        cairo_rectangle(cr, box.x, box.y, box.width, box.height);
    }
}

QTC_EXPORT void
clipRegion(cairo_t *cr, const cairo_region_t *region)
{
    cairo_new_path(cr);
    if (qtcLikely(region)) {
        pathRegion(cr, region);
        cairo_clip(cr);
    }
}

QTC_EXPORT void
clipRect(cairo_t *cr, const QtcRect *_rect)
{
    cairo_new_path(cr);
    if (qtcLikely(_rect)) {
        cairo_rectangle(cr, _rect->x, _rect->y, _rect->width, _rect->height);
        cairo_clip(cr);
    }
}

QTC_EXPORT void
setColor(cairo_t *cr, const GdkColor *col, double a)
{
    cairo_set_source_rgba(cr, col->red / 65535.0, col->green / 65535.0,
                          col->blue / 65535.0, a);
}

QTC_EXPORT void
patternAddColorStop(cairo_pattern_t *pt, double offset,
                    const GdkColor *col, double a)
{
    cairo_pattern_add_color_stop_rgba(pt, offset, col->red / 65535.0,
                                      col->green / 65535.0,
                                      col->blue / 65535.0, a);
}

QTC_EXPORT void
pathTopLeft(cairo_t *cr, double xd, double yd, double width,
            double height, double radius, ECornerBits round)
{
    bool rounded = radius > 0.0;

    if (rounded && round & CORNER_BL) {
        cairo_arc(cr, xd + radius, yd + height - radius, radius,
                  M_PI * 0.75, M_PI);
    } else {
        cairo_move_to(cr, xd, yd + height);
    }
    if (rounded && round & CORNER_TL) {
        cairo_arc(cr, xd + radius, yd + radius, radius, M_PI, M_PI * 1.5);
    } else {
        cairo_line_to(cr, xd, yd);
    }
    if (rounded && round & CORNER_TR) {
        cairo_arc(cr, xd + width - radius, yd + radius,
                  radius, M_PI * 1.5, M_PI * 1.75);
    } else {
        cairo_line_to(cr, xd + width, yd);
    }
}

QTC_EXPORT void
pathBottomRight(cairo_t *cr, double xd, double yd, double width,
                double height, double radius, ECornerBits round)
{
    bool rounded = radius > 0.0;

    if (rounded && round & CORNER_TR) {
        cairo_arc(cr, xd + width - radius, yd + radius, radius, M_PI * 1.75, 0);
    } else {
        cairo_move_to(cr, xd + width, yd);
    }
    if (rounded && round & CORNER_BR) {
        cairo_arc(cr, xd + width - radius, yd + height - radius,
                  radius, 0, M_PI * 0.5);
    } else {
        cairo_line_to(cr, xd + width, yd + height);
    }
    if (rounded && round & CORNER_BL) {
        cairo_arc(cr, xd + radius, yd + height - radius,
                  radius, M_PI * 0.5, M_PI * 0.75);
    } else {
        cairo_line_to(cr, xd, yd + height);
    }
}

QTC_EXPORT void
pathWhole(cairo_t *cr, double xd, double yd, double width,
          double height, double radius, ECornerBits round)
{
    bool rounded = radius > 0.0;

    if (rounded && round & CORNER_TL) {
        cairo_move_to(cr, xd + radius, yd);
    } else {
        cairo_move_to(cr, xd, yd);
    }
    if (rounded && round & CORNER_TR) {
        cairo_arc(cr, xd + width - radius, yd + radius, radius,
                  M_PI * 1.5, M_PI * 2);
    } else {
        cairo_line_to(cr, xd + width, yd);
    }
    if (rounded && round & CORNER_BR) {
        cairo_arc(cr, xd + width - radius, yd + height - radius,
                  radius, 0, M_PI * 0.5);
    } else {
        cairo_line_to(cr, xd + width, yd + height);
    }
    if (rounded && round & CORNER_BL) {
        cairo_arc(cr, xd + radius, yd + height - radius,
                  radius, M_PI * 0.5, M_PI);
    } else {
        cairo_line_to(cr, xd, yd + height);
    }
    if (rounded && round & CORNER_TL) {
        cairo_arc(cr, xd + radius, yd + radius, radius, M_PI, M_PI * 1.5);
    } else {
        cairo_line_to(cr, xd, yd);
    }
}

}
}
