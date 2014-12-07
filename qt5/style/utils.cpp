/*****************************************************************************
 *   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
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

#include <qtcurve-utils/log.h>
#include "utils.h"
#ifdef Q_WS_X11
#include <qtcurve-utils/x11utils.h>
#endif
#include <qtcurve-utils/qtutils.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QWindow>

#ifdef QTC_QT5_ENABLE_KDE
#  include <kdeversion.h>
#  include <KDE/KWindowSystem>
#endif

namespace QtCurve {
namespace Utils {

bool
compositingActive()
{
#ifndef QTC_QT5_ENABLE_KDE
#ifdef Q_WS_X11
    return qtcX11CompositingActive();
#else
    // this would be the safe bet:
    return false;
#endif
#else
    return KWindowSystem::compositingActive();
#endif
}

static inline WId
findWid(const QWidget *w)
{
    do {
        WId wid = qtcGetWid(w);
        if (wid || w->isWindow()) {
            return wid;
        }
    } while ((w = w->parentWidget()));
    return (WId)0;
}

static QWindow*
findWindowHandle(const QWidget *w)
{
    do {
        QWindow *window = w->windowHandle();
        if (window || w->isWindow()) {
            return window;
        }
    } while ((w = w->parentWidget()));
    return NULL;
}

bool
hasAlphaChannel(const QWidget *widget)
{
    if (!widget)
        return false;
    if (QWindow *window = findWindowHandle(widget)) {
        return window->format().alphaBufferSize() > 0;
    }
#ifdef Q_WS_X11
    if (WId wid = findWid(widget)) {
        return qtcX11HasAlpha(wid);
    }
#endif
    return compositingActive();
}

}
}
