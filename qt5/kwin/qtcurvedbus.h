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

#ifndef _QTCURVE_DBUS_H_
#define _QTCURVE_DBUS_H_

#include <QDBusAbstractAdaptor>
#include "qtcurvehandler.h"

namespace KWinQtCurve {

class QtCurveDBus : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.QtCurve")

    public:

    QtCurveDBus(QtCurveHandler *handler) : QDBusAbstractAdaptor(handler) { }

    void emitBorderSizes()              { emit borderSizesChanged(); }
    void emitMbToggle(unsigned int xid) { emit toggleMenuBar(xid); }
    void emitSbToggle(unsigned int xid) { emit toggleStatusBar(xid); }

    Q_SIGNALS:

    void borderSizesChanged();
    void toggleMenuBar(unsigned int xid);
    void toggleStatusBar(unsigned int xid);

    public Q_SLOTS:

    Q_NOREPLY void menuBarSize(unsigned int xid, int size)      { Handler()->menuBarSize(xid, size); }
    Q_NOREPLY void statusBarState(unsigned int xid, bool state) { Handler()->statusBarState(xid, state); }
};

}

#endif
