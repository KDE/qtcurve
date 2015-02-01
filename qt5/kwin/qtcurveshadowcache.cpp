/*****************************************************************************
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

//////////////////////////////////////////////////////////////////////////////
// Taken from: oxygenshadowcache.cpp
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

#include <kdeversion.h>

#include <KColorUtils>
#include <KColorScheme>
#include <QPainter>

#include <style/qtcurve.h>

#include "qtcurveshadowcache.h"
#include "qtcurveclient.h"
#include "qtcurvehandler.h"

namespace QtCurve {
namespace KWin {

static bool lowThreshold(const QColor &color)
{
    QColor darker = KColorScheme::shade(color, KColorScheme::MidShade, 0.5);
    return KColorUtils::luma(darker) > KColorUtils::luma(color);
}

static QColor backgroundTopColor(const QColor &color)
{

    if(lowThreshold(color)) return KColorScheme::shade(color, KColorScheme::MidlightShade, 0.0);
    qreal my = KColorUtils::luma(KColorScheme::shade(color, KColorScheme::LightShade, 0.0));
    qreal by = KColorUtils::luma(color);
    return KColorUtils::shade(color, (my - by) * 0.9/*_bgcontrast*/);

}

static QColor backgroundBottomColor(const QColor &color)
{
    QColor midColor = KColorScheme::shade(color, KColorScheme::MidShade, 0.0);
    if(lowThreshold(color)) return midColor;

    qreal by = KColorUtils::luma(color);
    qreal my = KColorUtils::luma(midColor);
    return KColorUtils::shade(color, (my - by) * 0.9/*_bgcontrast*/ * 0.85);
}

static QColor calcLightColor(const QColor &color)
{
    return KColorScheme::shade(color, KColorScheme::LightShade, 0.7/*_contrast*/);
}

QtCurveShadowCache::QtCurveShadowCache()
                  : m_activeShadowConfig(ShadowConfig(QPalette::Active))
                  , m_inactiveShadowConfig(ShadowConfig(QPalette::Inactive))
{
    m_shadowCache.setMaxCost(1<<6);
}

bool QtCurveShadowCache::shadowConfigChanged(const ShadowConfig &other) const
{
    const auto &local = (other.colorGroup() == QPalette::Active ?
                         m_activeShadowConfig : m_inactiveShadowConfig);
    return !(local == other);
}

void QtCurveShadowCache::setShadowConfig(const ShadowConfig &other)
{
    auto &local = (other.colorGroup() == QPalette::Active ?
                   m_activeShadowConfig : m_inactiveShadowConfig);
    local = other;
    reset();
}

TileSet*
QtCurveShadowCache::tileSet(const QtCurveClient *client, bool roundAllCorners)
{
    Key key(client);
    int hash(key.hash());

    if (m_shadowCache.contains(hash))
        return m_shadowCache.object(hash);

    qreal   size(shadowSize());
    TileSet *tileSet = new TileSet(shadowPixmap(client, key.active, roundAllCorners), size, size, 1, 1);

    m_shadowCache.insert(hash, tileSet);
    return tileSet;
}

QPixmap QtCurveShadowCache::shadowPixmap(const QtCurveClient *client, bool active, bool roundAllCorners) const
{
    Key      key(client);
    QPalette palette(client->widget()->palette());
    QColor   color(palette.color(client->widget()->backgroundRole()));

    return simpleShadowPixmap(color, active, roundAllCorners);
}

QPixmap QtCurveShadowCache::simpleShadowPixmap(const QColor &color, bool active, bool roundAllCorners) const
{
    static const qreal fixedSize = 25.5;

    const ShadowConfig &shadowConfig(active ? m_activeShadowConfig : m_inactiveShadowConfig);

    // offsets are scaled with the shadow size
    // so that the ratio Top-shadow/Bottom-shadow is kept constant when shadow size is changed
    qreal   size(shadowSize()),
            shadowSize(shadowConfig.shadowSize());
    QPixmap shadow(size*2, size*2);

    shadow.fill(Qt::transparent);

    QPainter p(&shadow);

    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    if (shadowSize) {
        if (ShadowConfig::SH_ACTIVE == shadowConfig.shadowType()) {
            {
                // inner (shark) gradient
                const qreal gradientSize = qMin(shadowSize,
                                                (shadowSize + fixedSize) / 2);
                const qreal hoffset = (shadowConfig.horizontalOffset() / 100. *
                                       gradientSize / fixedSize);
                const qreal voffset = (shadowConfig.verticalOffset() / 100. *
                                       gradientSize / fixedSize);

                QRadialGradient rg = QRadialGradient(size + 12. * hoffset,
                                                     size + 12. * voffset,
                                                     gradientSize);
                rg.setColorAt(1, Qt::transparent);

                // gaussian shadow is used
                int nPoints(10 * gradientSize / fixedSize);
                Gaussian f(0.85, 0.25);
                QColor c = shadowConfig.innerColor();
                for (int i = 0;i < nPoints;i++) {
                    qreal x = qreal(i) / nPoints;
                    c.setAlphaF(f(x));
                    rg.setColorAt(x, c);
                }

                p.setBrush(rg);
                renderGradient(p, shadow.rect(), rg, roundAllCorners);
            }

            {
                // outer (spread) gradient
                const qreal gradientSize = shadowSize;
                const qreal hoffset = (shadowConfig.horizontalOffset() / 100. *
                                       gradientSize / fixedSize);
                const qreal voffset = (shadowConfig.verticalOffset() / 100. *
                                       gradientSize / fixedSize);

                QRadialGradient rg = QRadialGradient(size + 12. * hoffset,
                                                     size + 12. * voffset,
                                                     gradientSize);
                rg.setColorAt(1, Qt::transparent);

                // gaussian shadow is used
                int nPoints(10 * gradientSize / fixedSize);
                Gaussian f(0.46, 0.42);
                QColor c = shadowConfig.outerColor();
                for (int i = 0;i < nPoints;i++) {
                    qreal x = qreal(i) / nPoints;
                    c.setAlphaF(f(x));
                    rg.setColorAt(x, c);
                }
                p.setBrush(rg);
                p.drawRect(shadow.rect());
            }
        } else {
            {
                // inner (sharp gradient)
                const qreal gradientSize = qMin(shadowSize, fixedSize);
                const qreal hoffset = (shadowConfig.horizontalOffset() / 100. *
                                       gradientSize / fixedSize);
                const qreal voffset = (shadowConfig.verticalOffset() / 100. *
                                       gradientSize / fixedSize);

                QRadialGradient rg = QRadialGradient(size + hoffset,
                                                     size + voffset,
                                                     gradientSize);
                rg.setColorAt(1, Qt::transparent);

                // parabolic shadow is used
                int nPoints(10 * gradientSize / fixedSize);
                Parabolic f(0.85, 0.22);
                QColor c = shadowConfig.outerColor();
                for (int i = 0;i < nPoints;i++) {
                    qreal x = qreal(i) / nPoints;
                    c.setAlphaF(f(x));
                    rg.setColorAt(x, c);
                }

                p.setBrush(rg);
                renderGradient(p, shadow.rect(), rg, roundAllCorners);
            }

            {
                // mid gradient
                const qreal gradientSize = qMin(shadowSize,
                                                (shadowSize +
                                                 2 * fixedSize) / 3);
                const qreal hoffset = (shadowConfig.horizontalOffset() / 100. *
                                       gradientSize / fixedSize);
                const qreal voffset = (shadowConfig.verticalOffset() / 100. *
                                       gradientSize / fixedSize);

                // gaussian shadow is used
                QRadialGradient rg = QRadialGradient(size + 8. * hoffset,
                                                     size + 8. * voffset,
                                                     gradientSize);
                rg.setColorAt(1, Qt::transparent);

                int nPoints(10 * gradientSize / fixedSize);
                Gaussian f(0.54, 0.21);
                QColor c = shadowConfig.outerColor();
                for (int i = 0;i < nPoints;i++) {
                    qreal x = qreal(i) / nPoints;
                    c.setAlphaF(f(x));
                    rg.setColorAt(x, c);
                }

                p.setBrush(rg);
                p.drawRect(shadow.rect());
            }
            {
                // outer (spread) gradient
                const qreal gradientSize = shadowSize;
                const qreal hoffset = (shadowConfig.horizontalOffset() / 100. *
                                       gradientSize / fixedSize);
                const qreal voffset = (shadowConfig.verticalOffset() / 100. *
                                       gradientSize / fixedSize);

                // gaussian shadow is used
                QRadialGradient rg = QRadialGradient(size + 20. * hoffset,
                                                     size + 20. * voffset,
                                                     gradientSize);
                rg.setColorAt(1, Qt::transparent);

                int nPoints(20 * gradientSize / fixedSize);
                Gaussian f(0.155, 0.445);
                QColor c = shadowConfig.outerColor();
                for (int i = 0;i < nPoints;i++) {
                    qreal x = qreal(i) / nPoints;
                    c.setAlphaF(f(x));
                    rg.setColorAt(x, c);
                }

                p.setBrush(rg);
                p.drawRect(shadow.rect());
            }
        }
    }

    // draw the corner of the window - actually all 4 corners as one circle
    // this is all fixedSize. Does not scale with shadow size
    QLinearGradient lg = QLinearGradient(0.0, size-4.5, 0.0, size+4.5);
    lg.setColorAt(0.0, calcLightColor(backgroundTopColor(color)));
    lg.setColorAt(0.51, backgroundBottomColor(color));
    lg.setColorAt(1.0, backgroundBottomColor(color));

    p.setBrush(lg);
    p.drawEllipse(QRectF(size-4, size-4, 8, 8));
    p.end();
    return shadow;
}

void QtCurveShadowCache::renderGradient(QPainter &p, const QRectF &rect, const QRadialGradient &rg, bool hasBorder) const
{
    if( hasBorder )
    {
        p.setBrush( rg );
        p.drawRect( rect );
        return;
    }


    qreal          size(rect.width() / 2.0),
                   hoffset(rg.center().x() - size),
                   voffset(rg.center().y() - size),
                   radius(rg.radius());
    QGradientStops stops(rg.stops());

    // draw ellipse for the upper rect
    {
        QRectF rect(hoffset, voffset, 2*size-hoffset, size);
        p.setBrush(rg);
        p.drawRect(rect);
    }

    // draw square gradients for the lower rect
    {
        // vertical lines
        QRectF          rect(hoffset, size+voffset, 2*size-hoffset, 4);
        QLinearGradient lg(hoffset, 0.0, 2*size+hoffset, 0.0);

        for(int i = 0; i<stops.size(); i++)
        {
            QColor c(stops[i].second);
            qreal  xx(stops[i].first*radius);

            lg.setColorAt((size - xx) / (2. * size), c);
            lg.setColorAt((size + xx) / (2. * size), c);
        }

        p.setBrush(lg);
        p.drawRect(rect);
    }

    {
        // horizontal line
        QRectF          rect(size-4+hoffset, size+voffset, 8, size);
        QLinearGradient lg = QLinearGradient(0, voffset, 0, 2*size+voffset);

        for(int i = 0; i<stops.size(); i++)
        {
            QColor c(stops[i].second);
            qreal  xx(stops[i].first*radius);

            lg.setColorAt((size + xx) / (2. * size), c);
        }

        p.setBrush(lg);
        p.drawRect(rect);
    }

    {
      // bottom-left corner
        QRectF          rect(hoffset, size+4+voffset, size-4, size);
        QRadialGradient rg = QRadialGradient(size+hoffset-4, size+4+voffset, radius);

        for(int i = 0; i<stops.size(); i++)
        {
            QColor c(stops[i].second);
            qreal  xx(stops[i].first -4.0/rg.radius());

            if(xx<0 && i < stops.size()-1)
            {
                qreal x1(stops[i+1].first -4.0/rg.radius());
                c = KColorUtils::mix(c, stops[i+1].second, -xx/(x1-xx));
                xx = 0;
            }

            rg.setColorAt(xx, c);
        }

        p.setBrush(rg);
        p.drawRect(rect);
    }

    {
      // bottom-right corner
        QRectF          rect(size+4+hoffset, size+4+voffset, size-4, size);
        QRadialGradient rg = QRadialGradient(size+hoffset+4, size+4+voffset, radius);

        for(int i = 0; i<stops.size(); i++)
        {
            QColor c(stops[i].second);
            qreal  xx(stops[i].first -4.0/rg.radius());

            if(xx<0 && i < stops.size()-1)
            {
                qreal x1(stops[i+1].first -4.0/rg.radius());
                c = KColorUtils::mix(c, stops[i+1].second, -xx/(x1-xx));
                xx = 0;
            }

            rg.setColorAt(xx, c);
        }

        p.setBrush(rg);
        p.drawRect(rect);
    }
}

QtCurveShadowCache::Key::Key(const QtCurveClient *client)
                       : active(client->isActive())
                       , isShade(client->isShade())
{
}

}
}
