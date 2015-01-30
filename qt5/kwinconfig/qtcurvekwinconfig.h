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

#ifndef QTCURVE_KWIN_CONFIG_WIDGET_H
#define QTCURVE_KWIN_CONFIG_WIDGET_H

#include "ui_qtcurvekwinconfigwidget.h"
#include "../kwin/qtcurveshadowconfiguration.h"
#include "../kwin/qtcurveconfig.h"

class KConfig;
class KConfigGroup;

class QtCurveKWinConfig: public QWidget, public Ui::QtCurveKWinConfigWidget {
    Q_OBJECT

public:
    QtCurveKWinConfig(KConfig *config, QWidget *parent);
    ~QtCurveKWinConfig();

    void setNote(const QString &txt);
    void load(KConfig *config);
    void save(KConfig *config);
    bool ok();

Q_SIGNALS:
    void changed();

public Q_SLOTS:
    void load(const KConfigGroup &);
    void save(KConfigGroup &);
    void defaults();

private:
    void activeShadowColorTypeChanged();
    void inactiveShadowColorTypeChanged();
    void innerBorderChanged();
    void outerBorderChanged();
    void setShadows();
    void setWidgets(const KWinQtCurve::QtCurveConfig &cfg);
    void setWidgetStates();
    void shadowsChanged();
    void sizeChanged();

    bool m_ok;
    KWinQtCurve::QtCurveShadowConfiguration m_activeShadows,
                                            m_inactiveShadows;
};

inline bool
QtCurveKWinConfig::ok()
{
    return m_ok;
}

#endif
