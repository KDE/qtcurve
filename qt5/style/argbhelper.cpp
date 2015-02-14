/*****************************************************************************
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

#include "argbhelper.h"
#include <qtcurve-utils/qtutils.h>

#include <QWindow>
#include "private/qwidget_p.h"

namespace QtCurve {
__attribute__((hot)) void
addAlphaChannel(QWidget *widget)
{
    QTC_RET_IF_FAIL(!qtcGetWid(widget));
    // Set this for better efficiency for now
    widget->setAutoFillBackground(false);
    QWindow *window = widget->windowHandle();
    QWidgetPrivate *widgetPrivate =
        static_cast<QWidgetPrivate*>(QObjectPrivate::get(widget));
    widgetPrivate->updateIsOpaque();
    if (!window) {
        widgetPrivate->createTLExtra();
        widgetPrivate->createTLSysExtra();
        window = widget->windowHandle();
    }
    if (window) {
        // Maybe we can register event filters and/or listen for signals
        // like parent change or screen change on the QWidgetWindow
        // so that we have a better change to update the alpha info
        QSurfaceFormat format = window->format();
        format.setAlphaBufferSize(8);
        window->setFormat(format);
    }
}

}
