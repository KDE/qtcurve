/*****************************************************************************
 *   Copyright 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>         *
 *   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#ifndef TILESET_H
#define TILESET_H

#include <QPixmap>
#include <QRect>
#include <QVector>

//! handles proper scaling of pixmap to match widget rect.
/*!
tilesets are collections of stretchable pixmaps corresponding to a given widget corners, sides, and center.
corner pixmaps are never stretched. center pixmaps are
*/
class TileSet
{
public:
   /**
    * Create a TileSet from a pixmap. The size of the bottom/right chunks is
    * whatever is left over from the other chunks, whose size is specified
    * in the required parameters.
    *
    * @param w1 width of the left chunks
    * @param h1 height of the top chunks
    * @param w2 width of the not-left-or-right chunks
    * @param h2 height of the not-top-or-bottom chunks
    */
    TileSet(const QPixmap&, int w1, int h1, int w2, int h2);

   /**
    * Create a TileSet from a pixmap. The size of the top/left and bottom/right
    * chunks is specified, with the middle chunks created from the specified
    * portion of the pixmap. This allows the middle chunks to overlap the outer
    * chunks (or to not use all pixels). The top/left and bottom/right chunks
    * are carved out of the corners of the pixmap.
    *
    * @param w1 width of the left chunks
    * @param h1 height of the top chunks
    * @param w3 width of the right chunks
    * @param h3 height of bottom chunks
    * @param x2 x-coordinate of the top of the not-left-or-right chunks
    * @param y2 y-coordinate of the left of the not-top-or-bottom chunks
    * @param w2 width of the not-left-or-right chunks
    * @param h2 height of the not-top-or-bottom chunks
    */
    TileSet(const QPixmap &pix, int w1, int h1, int w3, int h3, int x2, int y2, int w2, int h2);

    //! empty constructor
    TileSet();

    //! destructor
    virtual ~TileSet()
    {}

    /**
     * Flags specifying what sides to draw in ::render. Corners are drawn when
     * the sides forming that corner are drawn, e.g. Top|Left draws the
     * top-center, center-left, and top-left chunks. The center-center chunk is
     * only drawn when Center is requested.
     */
    enum Tile {
        Top = 0x1,
        Left = 0x2,
        Right = 0x8,
        Bottom = 0x4,
        Center = 0x10,
        Ring = 0x0f,
        Horizontal = 0x1a,
        Vertical = 0x15,
        Full = 0x1f
    };
    Q_DECLARE_FLAGS(Tiles, Tile)

    /**
     * Fills the specified rect with tiled chunks. Corners are never tiled,
     * edges are tiled in one direction, and the center chunk is tiled in both
     * directions. Partial tiles are used as needed so that the entire rect is
     * perfectly filled. Filling is performed as if all chunks are being drawn.
     */
    void render(const QRect&, QPainter*, Tiles = Ring) const;

protected:

    // initialize pixmap
    void initPixmap( int, const QPixmap&, int w, int h, const QRect &region);

    //! pixmap arry
    QVector<QPixmap> _pixmap;

    // dimensions
    int _w1;
    int _h1;
    int _w3;
    int _h3;

};

Q_DECLARE_OPERATORS_FOR_FLAGS(TileSet::Tiles)

#endif //TILESET_H
