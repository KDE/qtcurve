/*****************************************************************************
 *   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#ifndef __QTCURVESHADOWCACHE_H__
#define __QTCURVESHADOWCACHE_H__

//////////////////////////////////////////////////////////////////////////////
// Taken from: oxygenshadowcache.h
// handles caching of TileSet objects to draw shadows
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

//#define NEW_SHADOWS

#include "qtcurveshadowconfiguration.h"
#include "tileset.h"

#include <qtcurve-utils/number.h>

#include <QCache>
#include <QRadialGradient>

#include <cmath>

class QtCurveHelper;

namespace QtCurve {
namespace KWin {

class QtCurveClient;

class QtCurveShadowCache {
public:
    QtCurveShadowCache();

    const QColor&
    color(bool active)
    {
        return (active ? m_activeShadowConfig.color() :
                m_inactiveShadowConfig.color());
    }

    void
    invalidateCaches()
    {
        m_shadowCache.clear();
    }

    //! returns true if provided shadow configuration changes with respect to
    //  stored
    /*!
     * use ShadowConfig::colorRole() to decide whether it should be stored
     * as active or inactive
     */
    bool shadowConfigChanged(const ShadowConfig &other) const;

    //! set shadowConfiguration
    /*!
     * use ShadowConfig::colorRole() to decide whether it should be stored
     * as active or inactive
     */
    void setShadowConfig(const ShadowConfig &other);

    //! shadow size
    qreal
    shadowSize() const
    {
        qreal size(qMax(m_activeShadowConfig.shadowSize(),
                        m_inactiveShadowConfig.shadowSize()));

        // even if shadows are disabled, you need a minimum size to allow
        // corner rendering
        return qMax(size, 5.0);
    }

    TileSet *tileSet(const QtCurveClient *client, bool roundAllCorners);

    //! Key class to be used into QCache
    /*! class is entirely inline for optimization */
    class Key {
    public:
        explicit Key() : active(false), isShade(false) {}
        Key(const QtCurveClient *client);
        Key(int hash) : active((hash >> 1) & 1), isShade(hash & 1) {}

        int hash() const
        {
            return (active << 1) | isShade;
        }

        const bool active;
        const bool isShade;
    };

    class Parabolic {
    public:
        //! constructor
        Parabolic(qreal amplitude, qreal width)
            : m_amplitude(amplitude), m_width(width) {}
        //! value
        qreal
        operator() (qreal x) const
        {
            return qMax(0.0, m_amplitude * (1.0 - qtcSquare(x / m_width)));
        }
    private:
        const qreal m_amplitude;
        const qreal m_width;
    };

    class Gaussian {
    public:
        Gaussian(qreal amplitude, qreal width)
            : m_amplitude(amplitude), m_width(width) {}
        //! value
        qreal
        operator() (qreal x) const
        {
            return qMax(0.0, m_amplitude *
                        (std::exp(-qtcSquare(x / m_width) - 0.05)));
        }
    private:
        const qreal m_amplitude;
        const qreal m_width;
    };

    //! complex pixmap (when needed)
    QPixmap shadowPixmap(const QtCurveClient *client, bool active,
                         bool roundAllCorners) const;

    //! simple pixmap
    QPixmap simpleShadowPixmap(const QColor &color, bool active,
                               bool roundAllCorners) const;

    void reset() { m_shadowCache.clear(); }

private:
    //! draw gradient into rect
    /*! a separate method is used in order to properly account for corners */
    void renderGradient(QPainter &p, const QRectF &rect,
                        const QRadialGradient &rg, bool hasBorder) const;

    typedef QCache<int, TileSet> TileSetCache;

    ShadowConfig m_activeShadowConfig;
    ShadowConfig m_inactiveShadowConfig;
    TileSetCache m_shadowCache;
};

}
}

#endif
