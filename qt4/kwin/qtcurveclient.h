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

/*
  based on the window decoration "Plastik":
  Copyright (C) 2003-2005 Sandro Giessl <sandro@giessl.com>

  based on the window decoration "Web":
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>
 */

#ifndef QTCURVECLIENT_H
#define QTCURVECLIENT_H

#include "config.h"

#include <kdeversion.h>
#include <kcommondecoration.h>
#include <QPixmap>
#include <QColor>
#include "qtcurvehandler.h"

namespace QtCurve {
namespace KWin {

class QtCurveSizeGrip;
class QtCurveButton;
class QtCurveToggleButton;

class QtCurveClient : public KCommonDecorationUnstable {
    Q_OBJECT
public:
    QtCurveClient(KDecorationBridge *bridge, QtCurveHandler *factory);
    virtual ~QtCurveClient();

    QString visibleName() const;
    bool decorationBehaviour(DecorationBehaviour behaviour) const;
    int layoutMetric(LayoutMetric lm, bool respectWindowState=true,
                     const KCommonDecorationButton *btn=0) const;
    KCommonDecorationButton *createButton(ButtonType type);
    void init();
    void maximizeChange();
    void shadeChange();
    void activeChange();
    void captionChange();
    void reset(unsigned long changed);
    void paintEvent(QPaintEvent *e);
    void paintTitle(QPainter *painter, const QRect &capRect,
                    const QRect &alignFullRect, const QString &cap,
                    const QPixmap &pix, int shadowSize=0, bool isTab=false,
                    bool activeTab=false);
    void updateWindowShape();
    QRegion getMask(int round, const QRect &r) const;
    void updateCaption();
    bool eventFilter(QObject *o, QEvent *e);
    bool isMaximized() const
    {
        return (maximizeMode() == MaximizeFull &&
                !options()->moveResizeMaximizedWindows());
    }
    void menuBarSize(int size);
    void statusBarState(bool state);
    QtCurveToggleButton *createToggleButton(bool menubar);
    void informAppOfBorderSizeChanges();
    void sendToggleToApp(bool menubar);
public Q_SLOTS:
    void toggleMenuBar();
    void toggleStatusBar();

private:
    bool onlyMenuIcon(bool left) const;
    QRect captionRect() const;
    void createSizeGrip();
    void deleteSizeGrip();
    void informAppOfActiveChange();
    const QString &windowClass();

    struct ButtonBgnd {
        QPixmap pix;
        int app;
        QColor col;
    };

    static const int constNumButtonStates = 2;

    QtCurveSizeGrip *m_resizeGrip;
    ButtonBgnd m_buttonBackground[constNumButtonStates];
    QRect m_captionRect;
    QString m_caption;
    QString m_windowClass;
    QFont m_titleFont;
    int m_menuBarSize;
    QtCurveToggleButton *m_toggleMenuBarButton;
    QtCurveToggleButton *m_toggleStatusBarButton;
    // bool m_hover;
};

}
}

#endif
