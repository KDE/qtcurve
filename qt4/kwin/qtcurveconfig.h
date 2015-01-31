/*****************************************************************************
 *   Copyright 2010 Craig Drummond <craig.p.drummond@gmail.com>              *
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

#ifndef __QTCURVE_CONFIG_H__
#define __QTCURVE_CONFIG_H__

class KConfig;

namespace QtCurve {
namespace KWin {

class QtCurveConfig {
public:
    enum Size {
        BORDER_NONE = 0,
        BORDER_NO_SIDES,
        BORDER_TINY,
        BORDER_NORMAL,
        BORDER_LARGE,
        BORDER_VERY_LARGE,
        BORDER_HUGE,
        BORDER_VERY_HUGE,
        BORDER_OVERSIZED
    };

    enum Shade
    {
        SHADE_NONE,
        SHADE_DARK,
        SHADE_LIGHT,
        SHADE_SHADOW
    };

    QtCurveConfig()               { defaults(); }

    void defaults();
    void load(const KConfig *cfg, const char *grp=0L);
    void save(KConfig *cfg, const char *grp=0L);

    Size  borderSize() const        { return (Size)m_borderSize; }
    bool  roundBottom() const       { return m_roundBottom; }
    Shade outerBorder() const       { return m_outerBorder; }
    Shade innerBorder() const       { return m_innerBorder; }
    bool  borderlessMax() const     { return m_borderlessMax; }
    bool  customShadows() const     { return m_customShadows; }
    bool  grouping() const          { return m_grouping; }
    int   titleBarPad() const       { return m_titleBarPad; }
    int   opacity(bool a) const     { return a ? m_activeOpacity : m_inactiveOpacity; }
    bool  opaqueBorder() const      { return m_opaqueBorder; }
    int   edgePad() const           { return m_edgePad; }
    void  setBorderSize(Size v)     { m_borderSize=v; }
    void  setRoundBottom(bool v)    { m_roundBottom=v; }
    void  setOuterBorder(Shade v)   { m_outerBorder=v; }
    void  setInnerBorder(Shade v)   { m_innerBorder=v; }
    void  setBorderlessMax(bool v)  { m_borderlessMax=v; }
    void  setCustomShadows(bool v)  { m_customShadows=v; }
    void  setGrouping(bool v)       { m_grouping=v; }
    void  setTitleBarPad(int v)     { m_titleBarPad=v; }
    void  setOpacity(int v, bool a) { a ? m_activeOpacity=v : m_inactiveOpacity=v; }
    void  setOpaqueBorder(bool v)   { m_opaqueBorder=v; }
    void  setEdgePad(int v)         { m_edgePad=v; }

    bool operator==(const QtCurveConfig &o) const
    {
        return m_borderSize==o.m_borderSize &&
               m_roundBottom==o.m_roundBottom &&
               m_outerBorder==o.m_outerBorder &&
               m_innerBorder==o.m_innerBorder &&
               m_borderlessMax==o.m_borderlessMax &&
               m_customShadows==o.m_customShadows &&
               m_grouping==o.m_grouping &&
               m_titleBarPad==o.m_titleBarPad &&
               m_activeOpacity==o.m_activeOpacity &&
               m_inactiveOpacity==o.m_inactiveOpacity &&
               m_opaqueBorder==o.m_opaqueBorder &&
               m_edgePad==o.m_edgePad;
    }

    bool operator!=(const QtCurveConfig &o) const { return !(*this==o); }

    private:

    int   m_borderSize,
          m_activeOpacity,
          m_inactiveOpacity;
    bool  m_roundBottom,
          m_borderlessMax,
          m_customShadows,
          m_grouping,
          m_opaqueBorder;
    Shade m_outerBorder,
          m_innerBorder;
    int   m_titleBarPad,
          m_edgePad;
};

}
}

#endif
