/*****************************************************************************
 *   Copyright 2010 Craig Drummond <craig.p.drummond@gmail.com>              *
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

#include "qtcurveconfig.h"
#include <KConfigCore/KConfig>
#include <KConfigCore/KConfigGroup>

namespace QtCurve {
namespace KWin {

#define GROUP "General"

void QtCurveConfig::defaults()
{
    m_borderSize=BORDER_NORMAL;
    m_roundBottom=true;
    m_outerBorder=SHADE_NONE;
    m_innerBorder=SHADE_NONE;
    m_borderlessMax=false;
    m_customShadows=false;
    m_grouping=true;
    m_titleBarPad=0;
    m_activeOpacity=m_inactiveOpacity=100;
    m_opaqueBorder=true;
    m_edgePad=0;
}

#define READ_ENTRY(name, field) do {                    \
        field = group.readEntry(name, def.field);       \
    } while (0)

static QtCurveConfig::Shade readShade(KConfigGroup &group, const char *key)
{
    QString entry=group.readEntry(key, QString());

    if(entry.isEmpty() || QLatin1String("false")==entry)
        return QtCurveConfig::SHADE_NONE;
    if(QLatin1String("true")==entry)
        return QtCurveConfig::SHADE_DARK;
    int val=entry.toInt();
    if(val>QtCurveConfig::SHADE_NONE && val<=QtCurveConfig::SHADE_SHADOW)
        return (QtCurveConfig::Shade)val;
    return QtCurveConfig::SHADE_NONE;
}

void QtCurveConfig::load(const KConfig *cfg, const char *grp)
{
    KConfigGroup  group(cfg, grp ? grp : GROUP);
    QtCurveConfig def;

    if(group.hasKey("BorderSize"))
        m_borderSize=(Size)group.readEntry("BorderSize", (int)def.borderSize());
    else
    {
        KConfig      kwin("kwinrc");
        KConfigGroup style(&kwin, "Style");
        int          size=style.readEntry("BorderSize", 1);

        if(0==size) // KDecorationDefines::BorderTiny
        {
            if(group.readEntry("DrawBottom", false))
                m_borderSize=BORDER_NO_SIDES;
            else
                m_borderSize=BORDER_NONE;
        }
        else
            m_borderSize=(Size)(size+2);
    }

    if(m_borderSize<BORDER_NONE || m_borderSize>BORDER_OVERSIZED)
        m_borderSize=BORDER_NORMAL;
    READ_ENTRY("BorderlessMax", m_borderlessMax);
    READ_ENTRY("CustomShadows", m_customShadows);
    READ_ENTRY("Grouping", m_grouping);
    READ_ENTRY("TitleBarPad", m_titleBarPad);
    READ_ENTRY("ActiveOpacity", m_activeOpacity);
    READ_ENTRY("InactiveOpacity", m_inactiveOpacity);
    READ_ENTRY("OpaqueBorder", m_opaqueBorder);
    READ_ENTRY("EdgePad", m_edgePad);

    if(m_titleBarPad<-5 || m_titleBarPad>10)
        m_titleBarPad=0;
    if(m_edgePad<0 || m_edgePad>10)
        m_edgePad=0;
    if(BORDER_NONE==m_borderSize)
        m_roundBottom=false;
    else
        READ_ENTRY("RoundBottom", m_roundBottom);

    m_outerBorder=readShade(group, "OuterBorder");
    if(m_borderSize<BORDER_TINY || SHADE_NONE==m_outerBorder)
        m_innerBorder=SHADE_NONE;
    else
        m_innerBorder=readShade(group, "InnerBorder");

    if(m_activeOpacity<0 || m_activeOpacity>100)
        m_activeOpacity=100;
    if(m_inactiveOpacity<0 || m_inactiveOpacity>100)
        m_inactiveOpacity=100;
}

#define WRITE_ENTRY(name, field) do {           \
        if (def.field == field) {               \
            group.deleteEntry(name);            \
        } else {                                \
            group.writeEntry(name, field);      \
        }                                       \
    } while (0)

void QtCurveConfig::save(KConfig *cfg, const char *grp)
{
    KConfigGroup  group(cfg, grp ? grp : GROUP);
    QtCurveConfig def;

    //WRITE_ENTRY("BorderSize", m_borderSize);
    // Have to write BorderSize, because if not found we read the kwin setting - to be
    // compatible with QtCurve<1.4
    group.writeEntry("BorderSize", m_borderSize);
    WRITE_ENTRY("RoundBottom", m_roundBottom);
    group.writeEntry("OuterBorder", (int)m_outerBorder);
    group.writeEntry("InnerBorder", (int)m_innerBorder);
    WRITE_ENTRY("BorderlessMax", m_borderlessMax);
    WRITE_ENTRY("CustomShadows", m_customShadows);
    WRITE_ENTRY("Grouping", m_grouping);
    WRITE_ENTRY("TitleBarPad", m_titleBarPad);
    WRITE_ENTRY("ActiveOpacity", m_activeOpacity);
    WRITE_ENTRY("InactiveOpacity", m_inactiveOpacity);
    WRITE_ENTRY("OpaqueBorder", m_opaqueBorder);
    WRITE_ENTRY("EdgePad", m_edgePad);
}

}
}
