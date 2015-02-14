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
/*
  based on the window decoration "Plastik":
  Copyright (C) 2003-2005 Sandro Giessl <sandro@giessl.com>

  based on the window decoration "Web":
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>
 */

#ifndef KWIN_QTCURVE_H
#define KWIN_QTCURVE_H

#include <QFont>
#include <QApplication>
#include <QBitmap>
#include <kdeversion.h>
#include <kdecoration.h>
#include <kdecorationfactory.h>
#include "config.h"
#include "qtcurveconfig.h"
#include "qtcurveshadowcache.h"

class QStyle;

namespace QtCurve {
namespace KWin {

enum ButtonIcon {
    CloseIcon = 0,
    CloseTabIcon,
    MaxIcon,
    MaxRestoreIcon,
    MinIcon,
    HelpIcon,
    OnAllDesktopsIcon,
    NotOnAllDesktopsIcon,
    KeepAboveIcon,
    NoKeepAboveIcon,
    KeepBelowIcon,
    NoKeepBelowIcon,
    ShadeIcon,
    UnShadeIcon,
    MenuIcon,
    NumButtonIcons
};

class QtCurveClient;
class QtCurveDBus;

#if !KDE_IS_VERSION(4, 11, 0)
// KDecorationFactoryUnstable already does nothing before 4.11 and
// is removed in kde5.
#define _KDecorationFactoryBase KDecorationFactoryUnstable
#else
#define _KDecorationFactoryBase KDecorationFactory
#endif

class QtCurveHandler : public QObject, public _KDecorationFactoryBase {
    Q_OBJECT
public:
    QtCurveHandler();
    ~QtCurveHandler();
    void setStyle();
    bool reset(unsigned long changed) override;
    void setBorderSize();

    KDecoration *createDecoration(KDecorationBridge*) override;
    bool supports(Ability ability) const override;

    const QBitmap &buttonBitmap(ButtonIcon type, const QSize &size,
                                bool toolWindow);
    int
    titleHeight() const
    {
        return m_titleHeight;
    }
    int
    titleHeightTool() const
    {
        return m_titleHeightTool;
    }
    const QFont&
    titleFont()
    {
        return m_titleFont;
    }
    const QFont&
    titleFontTool()
    {
        return m_titleFontTool;
    }
    int borderSize(bool bot=false) const;
    bool
    showResizeGrip() const
    {
        return QtCurveConfig::BORDER_NONE == m_config.borderSize();
    }
    bool
    haveBottomBorder() const
    {
        return QtCurveConfig::BORDER_NONE!=m_config.borderSize();
    }
    bool
    roundBottom() const
    {
        return (m_config.roundBottom() &&
                m_config.borderSize() > QtCurveConfig::BORDER_NONE);
    }
    QtCurveConfig::Shade
    outerBorder() const
    {
        return m_config.outerBorder();
    }
    QtCurveConfig::Shade
    innerBorder() const
    {
        return m_config.innerBorder();
    }
    QStyle*
    wStyle() const
    {
        return m_style ? m_style : QApplication::style();
    }
    int borderEdgeSize() const;
    int
    titleBarPad() const
    {
        return m_config.titleBarPad();
    }
    int
    edgePad() const
    {
        return m_config.edgePad();
    }
    bool
    borderlessMax() const
    {
        return m_config.borderlessMax();
    }
    int
    opacity(bool active) const
    {
        return m_config.opacity(active);
    }
    bool
    opaqueBorder() const
    {
        return m_config.opaqueBorder();
    }
    bool
    customShadows() const
    {
        return m_config.customShadows();
    }
    QtCurveShadowCache&
    shadowCache()
    {
        return m_shadowCache;
    }
    bool
    grouping() const
    {
        return m_config.grouping();
    }
    void menuBarSize(unsigned int xid, int size);
    void statusBarState(unsigned int xid, bool state);
    void emitToggleMenuBar(int xid);
    void emitToggleStatusBar(int xid);
    void borderSizeChanged();
    void
    addClient(QtCurveClient *c)
    {
        m_clients.append(c);
    }
    void removeClient(QtCurveClient *c);
    bool wasLastMenu(unsigned int id)
    {
        return id == m_lastMenuXid;
    }
    bool
    wasLastStatus(unsigned int id)
    {
        return id == m_lastStatusXid;
    }
    const QColor&
    hoverCol(bool active)
    {
        return m_hoverCols[active ? 1 : 0];
    }
private:
    bool readConfig(bool compositingToggled=false);

    int m_borderSize;
    int m_titleHeight;
    int m_titleHeightTool;
    int m_timeStamp;
    unsigned int m_lastMenuXid;
    unsigned int m_lastStatusXid;
    QFont m_titleFont;
    QFont m_titleFontTool;
    QStyle *m_style;
    QBitmap m_bitmaps[2][NumButtonIcons];
    QtCurveConfig m_config;
    QList<QtCurveClient*> m_clients;
    QtCurveDBus *m_dBus;
    QColor m_hoverCols[2];
    QtCurveShadowCache m_shadowCache;
};
QtCurveHandler *Handler();
}
}

#endif
