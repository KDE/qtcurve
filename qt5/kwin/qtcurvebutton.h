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

#ifndef QTCURVEBUTTON_H
#define QTCURVEBUTTON_H

#include <QImage>
#include "qtcurvehandler.h"
#include <kcommondecoration.h>

class QTimer;
class QStyle;

namespace QtCurve {
namespace KWin {

class QtCurveClient;

class QtCurveButton: public KCommonDecorationButton {
public:
    QtCurveButton(ButtonType type, QtCurveClient *parent);
    ~QtCurveButton() {}

    void reset(unsigned long changed);
    QtCurveClient*
    client()
    {
        return m_client;
    }

protected:
    void paintEvent(QPaintEvent *);

private:
    void enterEvent(QEvent *e);
    void leaveEvent(QEvent *e);
    void drawButton(QPainter *painter);
    void updateMask();

private:
    QtCurveClient *m_client;
    ButtonIcon m_iconType;
    bool m_hover;

    friend class IconEngine;
};

/**
 * This class creates bitmaps which can be used as icons on buttons. The icons
 * are "hardcoded".
 * Over the previous "Gimp->xpm->QImage->recolor->SmoothScale->QPixmap" solution
 * it has the important advantage that icons are more scalable and at the same
 * time sharp and not blurred.
 */
class IconEngine {
public:
    static QBitmap icon(ButtonIcon icon, int size, QStyle *style);

private:
    enum Object {
        HorizontalLine,
        VerticalLine,
        DiagonalLine,
        CrossDiagonalLine
    };

    static void drawObject(QPainter &p, Object object, int x, int y,
                           int length, int lineWidth);
};

}
}

#endif
