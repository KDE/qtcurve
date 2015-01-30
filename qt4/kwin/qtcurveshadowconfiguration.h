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

#ifndef __QTCURVESHADOWCONFIGURATION_H__
#define __QTCURVESHADOWCONFIGURATION_H__

#include <QPalette>
#ifdef midColor
#undef midColor
#endif

class KConfig;

namespace KWinQtCurve {

class QtCurveShadowConfiguration {
public:
    enum ColorType {
        CT_FOCUS = 0,
        CT_HOVER = 1,
        CT_SELECTION = 2,
        CT_TITLEBAR = 3,
        CT_GRAY = 4,
        CT_CUSTOM = 5
    };

    enum ShadowType {
        SH_ACTIVE = 0,
        SH_INACTIVE = 1
    };

    enum {
        MIN_SIZE = 10,
        MAX_SIZE = 100,
        MIN_OFFSET = 0,
        MAX_OFFSET = 20
    };

    QtCurveShadowConfiguration(QPalette::ColorGroup);

    virtual ~QtCurveShadowConfiguration() { }

    void                 defaults();
    void                 load(KConfig *cfg);
    void                 save(KConfig *cfg);

    QPalette::ColorGroup colorGroup() const           { return m_colorGroup; }
    int                  shadowSize() const           { return m_size; }
    void                 setShadowSize(int v)         { m_size = v; }
    int                  horizontalOffset() const     { return m_hOffset; }
    void                 setHorizontalOffset(int v)   { m_hOffset = v; }
    int                  verticalOffset() const       { return m_vOffset; }
    void                 setVerticalOffset(int v)     { m_vOffset = v; }
    void                 setColor(const QColor &c)    { m_color=c; }
    const QColor &       color() const                { return m_color; }
    ColorType            colorType() const            { return (ColorType)m_colorType; }
    void                 setColorType(ColorType ct);
    ShadowType           shadowType() const           { return (ShadowType)m_shadowType; }
    void                 setShadowType(ShadowType st) { m_shadowType=st; }

    // Keep compatible with Oxygen Shadow Cache code...
    const QColor &       innerColor() const           { return m_color; }
    const QColor &       midColor() const             { return m_color; }
    const QColor &       outerColor() const           { return m_color; }

    bool operator == (const QtCurveShadowConfiguration& other) const
    {
        return  m_colorGroup == other.m_colorGroup &&
                m_size == other.m_size &&
                m_hOffset == other.m_hOffset &&
                m_vOffset == other.m_vOffset &&
                m_color == other.m_color &&
                m_shadowType == other.m_shadowType;
    }

private:
    QPalette::ColorGroup m_colorGroup;
    int                  m_size,
                         m_hOffset,
                         m_vOffset,
                         m_colorType,
                         m_shadowType;
    QColor               m_color;
};

}

#endif
