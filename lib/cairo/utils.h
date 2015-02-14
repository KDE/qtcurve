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

#ifndef __QTC_CAIRO_UTILS_H__
#define __QTC_CAIRO_UTILS_H__

#include <qtcurve-utils/utils.h>
#include <qtcurve-utils/options.h>
#include <cairo.h>

#ifdef QTC_UTILS_GTK
#include <gdk/gdk.h>
#endif

struct _GdkPoint;
struct _GdkColor;
// cairo_rectangle_int_t is toooo long...
typedef cairo_rectangle_int_t QtcRect;
/**
 * Construct a QtcRect
 */
#define qtcRect(x, y, w, h) ((QtcRect){(x), (y), (w), (h)})

namespace QtCurve {
namespace Rect {

/**
 * \param a a QtcRect
 * \param b a QtcRect
 *
 * Test if rectangle \param a and rectangle \param b are equal.
 */
QTC_ALWAYS_INLINE static inline bool
equal(const QtcRect &a, const QtcRect &b)
{
    return (a.x == b.x && a.y == b.y && a.width == b.width &&
            a.height == b.height);
}

/**
 * \param src1 a QtcRect
 * \param src2 a QtcRect
 * \param[out] dest return location for the union
 *                  of \param src1 and \param src2
 *
 * Calculates the union of two rectangles.
 * The union of rectangles \param src1 and \param src2 is the smallest
 * rectangle which includes both \param src1 and \param src2 within it.
 * It is allowed for \param dest to be the same as either
 * \param src1 or \param src2.
 * \sa gdk_rectangle_union
 */
void union_(const QtcRect *src1, const QtcRect *src2, QtcRect *dest);

/**
 * \param src1 a QtcRect
 * \param src2 a QtcRect
 * \param[out] dest return location for the intersection of \param src1 and
 *                  \param src2, or NULL
 *
 * Calculates the intersection of two rectangles. It is allowed for
 * \param dest to be the same as either \param src1 or \param src2.
 * If the rectangles do not intersect, \param dest's width and height is set
 * to 0 and its x and y values are undefined. If you are only interested in
 * whether the rectangles intersect, but not in the intersecting area itself,
 * pass %NULL for \param dest.
 *
 * \return true if the rectangles intersect.
 * \sa gdk_rectangle_intersect
 */
bool intersect(const QtcRect *src1, const QtcRect *src2, QtcRect *dest);

/**
 * \param rect a QtcRect
 * \param con a QtcRect
 *
 * Constrain \param to be the part within \param con.
 * \sa qtcRectIntersect
 */
QTC_ALWAYS_INLINE static inline void
constrain(QtcRect *_rect, const QtcRect *con)
{
    intersect(_rect, con, _rect);
}

}

namespace Cairo {

class Saver {
    Saver(const Saver&) = delete;
public:
    inline
    Saver(cairo_t *cr)
        : m_cr(cr),
          m_restored(true)
    {
        save();
    }
    inline void
    restore()
    {
        // not thread safe but we don't care...
        if (!m_restored) {
            m_restored = true;
            cairo_restore(m_cr);
        }
    }
    inline void
    save()
    {
        // not thread safe but we don't care...
        if (m_restored) {
            m_restored = false;
            cairo_save(m_cr);
        }
    }
    inline void
    resave()
    {
        // not thread safe but we don't care...
        if (!m_restored) {
            restore();
            save();
        }
    }
    inline
    ~Saver()
    {
        restore();
    }
private:
    cairo_t *const m_cr;
    bool m_restored;
};

/**
 * \param cr a cairo_t
 * \param pts an array of GdkPoint
 * \param count the number of points in \param pts
 *
 * Add points in \param pts to the current path of \param cr.
 * The path is shifted by (0.5, 0.5) so that it is in the middle of the pixel.
 */
void pathPoints(cairo_t *cr, const _GdkPoint *pts, int count);

/**
 * \param cr a cairo_t
 * \param region a cairo_region_t
 *
 * Clip \param cr on \param region. The current path of \param cr will
 * be cleared.
 */
void clipRegion(cairo_t *cr, const cairo_region_t *region);

/**
 * \param cr a cairo_t
 * \param rect a QtcRect
 *
 * Clip \param cr on \param rect. The current path of \param cr will
 * be cleared.
 */
void clipRect(cairo_t *cr, const QtcRect *_rect);

/**
 * \param cr a cairo_t
 * \param col the color to be set
 * \param a (optional) the opacity of the color in the range [0.0 .. 1.0]
 *          (default: 1).
 *
 * Set the source of \param cr to \param col with opacity \param a
 * \sa cairo_set_source_rgba
 */
void setColor(cairo_t *cr, const _GdkColor *col, double a=1);

/**
 * \param pt a cairo_pattern_t
 * \param offset an offset in the range [0.0 .. 1.0]
 * \param col the color to be added.
 * \param a (optional) the opacity of the color in the range [0.0 .. 1.0]
 *          (default: 1).
 *
 * Adds a color stop with color \param col and opacity \param a to
 * a gradient pattern at \param offset.
 * \sa cairo_pattern_add_color_stop_rgba
 */
void patternAddColorStop(cairo_pattern_t *pt, double offset,
                         const _GdkColor *col, double a=1);

void pathTopLeft(cairo_t *cr, double xd, double yd, double width,
                 double height, double radius, ECornerBits round);
void pathBottomRight(cairo_t *cr, double xd, double yd, double width,
                     double height, double radius, ECornerBits round);
void pathWhole(cairo_t *cr, double xd, double yd, double width,
               double height, double radius, ECornerBits round);

QTC_ALWAYS_INLINE static inline void
clipWhole(cairo_t *cr, double x, double y, int w, int h,
          double radius, ECornerBits round)
{
    cairo_new_path(cr);
    pathWhole(cr, x, y, w, h, radius, round);
    cairo_clip(cr);
}

#ifdef QTC_UTILS_GTK
QTC_ALWAYS_INLINE static inline cairo_t*
gdkCreateClip(GdkWindow *window, const QtcRect *area, double width=1)
{
    cairo_t *cr = gdk_cairo_create(window);
    clipRect(cr, area);
    cairo_set_line_width(cr, width);
    return cr;
}

QTC_ALWAYS_INLINE static inline cairo_t*
gdkCreateClip(GdkWindow *window, const GdkRectangle *area, double width=1)
{
    return gdkCreateClip(window, (const QtcRect*)area, width);
}
#endif

}
}

#endif
