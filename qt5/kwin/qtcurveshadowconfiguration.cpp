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

#include <kdeversion.h>
#if KDE_IS_VERSION(4, 3, 0)

#include <kcolorscheme.h>

#include <KConfig>
#include <KConfigGroup>
#include <QApplication>
#include "qtcurveshadowconfiguration.h"

namespace KWinQtCurve
{

QtCurveShadowConfiguration::QtCurveShadowConfiguration(QPalette::ColorGroup colorGroup)
                          : m_colorGroup(colorGroup)
{
    defaults();
}

void QtCurveShadowConfiguration::defaults()
{
    m_hOffset = 0;
    m_vOffset = 5;
    if(QPalette::Active==m_colorGroup)
    {
        m_size = 35;
        setColorType(CT_FOCUS);
        m_shadowType = SH_ACTIVE;
    }
    else
    {
        m_size = 30;
        setColorType(CT_GRAY);
        m_shadowType = SH_INACTIVE;
    }
}

void QtCurveShadowConfiguration::setColorType(ColorType ct)
{
    m_colorType = ct;
    switch (m_colorType) {
    default:
    case CT_FOCUS:
        m_color = KColorScheme(m_colorGroup)
            .decoration(KColorScheme::FocusColor).color();
        break;
    case CT_HOVER:
        m_color = KColorScheme(m_colorGroup)
            .decoration(KColorScheme::HoverColor).color();
        break;
    case CT_SELECTION:
        m_color = QApplication::palette().color(m_colorGroup,
                                                QPalette::Highlight);
        break;
    case CT_TITLEBAR: {
        m_color = KColorScheme(m_colorGroup, KColorScheme::Window)
            .background(m_colorGroup == QPalette::Active ?
                        KColorScheme::ActiveBackground :
                        KColorScheme::NormalBackground)
            .color();
        break;
    }
    case CT_GRAY:
        m_color = QColor("#393835");
        break;
    case CT_CUSTOM:
        break;
    }
}

#define CFG_GROUP (QPalette::Active==m_colorGroup ? "ActiveShadows" : "InactiveShadows")

#define READ_ENTRY(name, field) do {                    \
        field = group.readEntry(name, def.field);       \
    } while (0)

void QtCurveShadowConfiguration::load(KConfig *cfg)
{
    KConfigGroup               group(cfg, CFG_GROUP);
    QtCurveShadowConfiguration def(m_colorGroup);

    READ_ENTRY("Size", m_size);
    READ_ENTRY("HOffset", m_hOffset);
    READ_ENTRY("VOffset", m_vOffset);
    READ_ENTRY("ColorType", m_colorType);
    READ_ENTRY("ShadowType", m_shadowType);

    if(CT_CUSTOM==m_colorType)
        READ_ENTRY("Color", m_color);
    if(m_size<MIN_SIZE || m_size>MAX_SIZE)
        m_size=def.shadowSize();
    if(m_hOffset<MIN_OFFSET || m_hOffset>MAX_OFFSET)
        m_hOffset=def.horizontalOffset();
    if(m_vOffset<MIN_OFFSET || m_vOffset>MAX_OFFSET)
        m_vOffset=def.verticalOffset();
    setColorType((ColorType)m_colorType);
}

#define WRITE_ENTRY(name, field) do {           \
        if (def.field == field) {               \
            group.deleteEntry(name);            \
        } else {                                \
            group.writeEntry(name, field);      \
        }                                       \
    } while (0)

void QtCurveShadowConfiguration::save(KConfig *cfg)
{
    KConfigGroup               group(cfg, CFG_GROUP);
    QtCurveShadowConfiguration def(m_colorGroup);

    WRITE_ENTRY("Size", m_size);
    WRITE_ENTRY("HOffset", m_hOffset);
    WRITE_ENTRY("VOffset", m_vOffset);
    WRITE_ENTRY("ColorType", m_colorType);
    WRITE_ENTRY("ShadowType", m_shadowType);

    if (m_colorType != CT_CUSTOM) {
        group.deleteEntry("Color");
    } else {
        WRITE_ENTRY("Color", m_color);
    }
}

}

#endif
