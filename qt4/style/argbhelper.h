/*****************************************************************************
 *   Copyright 2014 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef __ARGBHELPER_H__
#define __ARGBHELPER_H__

#include <QtGlobal>
#ifdef Q_WS_X11
#  include <QX11Info>
#else
class QX11Info {
};
#endif
#include <qtcurve-utils/utils.h>

class QWidget;

namespace QtCurve {
// TODO, turn this into a complete helper class.
//     The QtcX11Info struct and the <QX11Info> include are in the header file
//     only for QtcX11Info::creatingDummy. Should move to argbhelper.cpp once
//     the real helper class is done.

// Access protected functions.
struct QtcX11Info: public QX11Info {
    static bool creatingDummy;
private:
    static QtcX11Info *getInfo(const QWidget *w);
    QWidget *rgbaDummy();
    void fixVisual();
    void setRgba();
    friend void fixVisual(QWidget *widget);
    friend void addAlphaChannel(QWidget *widget);
};

void fixVisual(QWidget *widget);
void addAlphaChannel(QWidget *widget);

}

#endif
