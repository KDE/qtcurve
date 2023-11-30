/*****************************************************************************
 *   Copyright 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>         *
 *   Copyright 2008 Long Huynh Huu <long.upcase@gmail.com>                   *
 *   Copyright 2008 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#include "tileset.h"

#include <kdeversion.h>

#include <QPainter>

void
TileSet::initPixmap(int s, const QPixmap &pix, int w, int h,
                    const QRect &region)
{
    if (w != region.width() || h != region.height()) {
        QPixmap tile = pix.copy(region);
        _pixmap[s] = QPixmap(w, h);
        _pixmap[s].fill(QColor(0, 0, 0, 0));
        QPainter p(&_pixmap[s]);
        p.drawTiledPixmap(0, 0, w, h, tile);
    } else {
        _pixmap[s] = pix.copy(region);
    }
}

TileSet::TileSet()
    : _w1(0), _h1(0), _w3(0), _h3(0)
{
}

TileSet::TileSet(const QPixmap &pix, int w1, int h1, int w2, int h2)
    : _w1(w1), _h1(h1), _w3(0), _h3(0)
{
    if (pix.isNull())
        return;

    _w3 = pix.width() - (w1 + w2);
    _h3 = pix.height() - (h1 + h2);
    int w = w2;
    while (w < 32 && w2 > 0)
        w += w2;
    int h = h2;
    while (h < 32 && h2 > 0)
        h += h2;

    // initialise pixmap array
    _pixmap.resize(9);
    initPixmap(0, pix, _w1, _h1, QRect(0, 0, _w1, _h1));
    initPixmap(1, pix, w, _h1, QRect(_w1, 0, w2, _h1));
    initPixmap(2, pix, _w3, _h1, QRect(_w1 + w2, 0, _w3, _h1));
    initPixmap(3, pix, _w1, h, QRect(0, _h1, _w1, h2));
    initPixmap(4, pix, w, h, QRect(_w1, _h1, w2, h2));
    initPixmap(5, pix, _w3, h, QRect(_w1 + w2, _h1, _w3, h2));
    initPixmap(6, pix, _w1, _h3, QRect(0, _h1 + h2, _w1, _h3));
    initPixmap(7, pix, w, _h3, QRect(_w1, _h1 + h2, w2, _h3));
    initPixmap(8, pix, _w3, _h3, QRect(_w1 + w2, _h1 + h2, _w3, _h3));
}

TileSet::TileSet(const QPixmap &pix, int w1, int h1, int w3, int h3,
                 int x1, int y1, int w2, int h2)
    : _w1(w1), _h1(h1), _w3(w3), _h3(h3)
{
    if (pix.isNull())
        return;

    int x2 = pix.width() - _w3;
    int y2 = pix.height() - _h3;
    int w = w2;
    while (w < 32 && w2 > 0)
        w += w2;
    int h = h2;
    while (h < 32 && h2 > 0)
        h += h2;

    // initialise pixmap array
    _pixmap.resize(9);
    initPixmap(0, pix, _w1, _h1, QRect(0, 0, _w1, _h1));
    initPixmap(1, pix, w, _h1, QRect(x1, 0, w2, _h1));
    initPixmap(2, pix, _w3, _h1, QRect(x2, 0, _w3, _h1));
    initPixmap(3, pix, _w1, h, QRect(0, y1, _w1, h2));
    initPixmap(4, pix, w, h, QRect(x1, y1, w2, h2));
    initPixmap(5, pix, _w3, h, QRect(x2, y1, _w3, h2));
    initPixmap(6, pix, _w1, _h3, QRect(0, y2, _w1, _h3));
    initPixmap(7, pix, w, _h3, QRect(x1, y2, w2, _h3));
    initPixmap(8, pix, _w3, _h3, QRect(x2, y2, _w3, _h3));
}

inline bool
bits(TileSet::Tiles flags, TileSet::Tiles testFlags)
{
    return (flags & testFlags) == testFlags;
}

void
TileSet::render(const QRect &r, QPainter *p, Tiles t) const
{
    // check initialization
    if(_pixmap.size() < 9)
        return;

    int x0, y0, w, h;
    r.getRect(&x0, &y0, &w, &h);

    // calculate pixmaps widths
    qreal wRatio(qreal(_w1) / qreal(_w1 + _w3));
    int wLeft = (t&Right) ? qMin(_w1, int(w * wRatio)) : _w1;
    int wRight = (t&Left) ? qMin(_w3, int(w * (1.0 - wRatio))) : _w3;

    // calculate pixmap heights
    qreal hRatio(qreal(_h1) / qreal(_h1 + _h3));
    int hTop = (t & Bottom) ? qMin(_h1, int(h * hRatio)) : _h1;
    int hBottom = (t & Top) ? qMin(_h3, int(h * (1.0 - hRatio))) : _h3;

    // calculate corner locations

    // maybe one should make the following two lines depend on
    // what tilesets are selected. Would be more logical, but would
    // probably break code all over the place ...
    w -= wLeft + wRight;
    h -= hTop + hBottom;
    int x1 = x0 + wLeft;
    int x2 = x1 + w;
    int y1 = y0 + hTop;
    int y2 = y1 + h;

    // corner
    if (bits(t, Top | Left))
        p->drawPixmap(x0, y0, _pixmap.at(0), 0, 0, wLeft, hTop);
    if (bits(t, Top | Right))
        p->drawPixmap(x2, y0, _pixmap.at(2), _w3 - wRight, 0, wRight, hTop);
    if (bits(t, Bottom | Left))
        p->drawPixmap(x0, y2, _pixmap.at(6), 0, _h3 - hBottom, wLeft, hBottom);
    if (bits(t, Bottom | Right))
        p->drawPixmap(x2, y2, _pixmap.at(8), _w3 - wRight, _h3 - hBottom,
                      wRight, hBottom);

    // top and bottom
    if (w > 0) {
        if (t & Top)
            p->drawTiledPixmap(x1, y0, w, hTop, _pixmap.at(1));
        if (t & Bottom) {
            p->drawTiledPixmap(x1, y2, w, hBottom, _pixmap.at(7),
                               0, _h3 - hBottom);
        }
    }

    // left and right
    if (h > 0) {
        if (t & Left)
            p->drawTiledPixmap(x0, y1, wLeft, h, _pixmap.at(3));
        if (t & Right) {
            p->drawTiledPixmap(x2, y1, wRight, h, _pixmap.at(5),
                               _w3 - wRight, 0);
        }
    }

    // center
    if ((t & Center) && h > 0 && w > 0) {
        p->drawTiledPixmap(x1, y1, w, h, _pixmap.at(4));
    }
}
