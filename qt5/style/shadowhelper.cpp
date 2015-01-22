/*****************************************************************************
 *   Copyright 2010 Craig Drummond <craig.p.drummond@gmail.com>              *
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

//////////////////////////////////////////////////////////////////////////////
// shadowhelper.h
// handle shadow _pixmaps passed to window manager via X property
// -------------------
//
// Copyright (c) 2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "shadowhelper.h"
#include "utils.h"

#include <QDockWidget>
#include <QMenu>
#include <QPainter>
#include <QToolBar>
#include <QEvent>

#include <qtcurve-utils/x11shadow.h>
#include <qtcurve-utils/qtprops.h>

namespace QtCurve {
const char *const ShadowHelper::netWMForceShadowPropertyName =
    "_KDE_NET_WM_FORCE_SHADOW";
const char *const ShadowHelper::netWMSkipShadowPropertyName =
    "_KDE_NET_WM_SKIP_SHADOW";

bool
ShadowHelper::registerWidget(QWidget *widget, bool force)
{
    QtcQWidgetProps props(widget);
    // make sure widget is not already registered
    if (props->shadowRegistered)
        return false;
    // check if widget qualifies
    if (!(force || acceptWidget(widget)))
        return false;
    props->shadowRegistered = true;
    // WinIdChange Event, should not happen though
    widget->installEventFilter(this);
    installX11Shadows(widget);
    return true;
}

void
ShadowHelper::unregisterWidget(QWidget *widget)
{
    QtcQWidgetProps props(widget);
    if (props->shadowRegistered) {
        uninstallX11Shadows(widget);
        props->shadowRegistered = false;
    }
}

bool
ShadowHelper::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::WinIdChange)
        installX11Shadows(static_cast<QWidget*>(object));
    return false;
}

bool
ShadowHelper::acceptWidget(QWidget *widget) const
{
    if (widget->property(netWMSkipShadowPropertyName).toBool())
        return false;
    if (widget->property(netWMForceShadowPropertyName).toBool())
        return true;

    // menus
    if (qobject_cast<QMenu*>(widget))
        return true;

    // combobox dropdown lists
    if (widget->inherits("QComboBoxPrivateContainer"))
        return true;

    // tooltips
    if ((widget->windowType() == Qt::ToolTip ||
         widget->inherits("QTipLabel")) && !widget->inherits("Plasma::ToolTip"))
        return true;

    // detached widgets
    if (qobject_cast<QToolBar*>(widget) || qobject_cast<QDockWidget*>(widget))
        return true;

    // reject
    return false;
}

bool
ShadowHelper::installX11Shadows(QWidget *widget)
{
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    QTC_RET_IF_FAIL(qtcX11Enabled(), false);
    if (WId wid = qtcGetWid(widget)) {
        if (widget->windowType() == Qt::ToolTip &&
            widget->inherits("QBalloonTip")) {
            bool atTop = true;
            int margin = qtcGetBalloonMargin(widget, &atTop);
            int margins[4] = {0, 0, 0, 0};
            // KWin's shadows margin order is top, right, bottom, left..
            margins[atTop ? 0 : 2] = margin;
            qtcX11ShadowInstall(wid, margins);
        } else {
            qtcX11ShadowInstall(wid);
        }
        return true;
    }
    return false;
}

void
ShadowHelper::uninstallX11Shadows(QWidget *widget) const
{
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    QTC_RET_IF_FAIL(qtcX11Enabled());
    if (WId wid = qtcGetWid(widget)) {
        qtcX11ShadowUninstall(wid);
    }
}
}
