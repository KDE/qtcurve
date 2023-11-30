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

#ifndef QTCURVETOGGLEBUTTON_H
#define QTCURVETOGGLEBUTTON_H

#include <kcommondecoration.h>

class QStyle;

namespace QtCurve {
namespace KWin {

class QtCurveClient;

class QtCurveToggleButton: public KCommonDecorationButton {
public:
    QtCurveToggleButton(bool menubar, QtCurveClient *parent);
    ~QtCurveToggleButton() { }

    void reset(unsigned long changed);
    QtCurveClient * client() { return m_client; }

    protected:

    void paintEvent(QPaintEvent *);

    private:

    void enterEvent(QEvent *e);
    void leaveEvent(QEvent *e);
    void drawButton(QPainter *painter);
    void updateMask();

    private:

    QtCurveClient *m_client;
    bool          isMenuBar,
                  m_hover;
};

}
}

#endif
