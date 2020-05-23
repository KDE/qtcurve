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

#include <qtcurve-utils/log.h>
#include "utils.h"
#include <qtcurve-utils/x11utils.h>
#include <qtcurve-utils/qtutils.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QWindow>

#ifdef QTC_QT5_ENABLE_KDE
#  include <kwindowsystem.h>
#endif

namespace QtCurve {
namespace Utils {

bool
compositingActive()
{
#ifndef QTC_QT5_ENABLE_KDE
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    return qtcX11CompositingActive();
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
    return nullptr;
}

bool
hasAlphaChannel(const QWidget *widget)
{
    if (!widget)
        return false;
    if (QWindow *window = findWindowHandle(widget)) {
        return window->format().alphaBufferSize() > 0;
    }
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    if (qtcX11Enabled()) {
        if (WId wid = findWid(widget)) {
            return qtcX11HasAlpha(wid);
        }
    }
    return compositingActive();
}

}
}
