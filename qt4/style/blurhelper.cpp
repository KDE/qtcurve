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

//////////////////////////////////////////////////////////////////////////////
// blurhelper.cpp
// handle regions passed to kwin for blurring
// -------------------
//
// Copyright (c) 2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
//
// Loosely inspired (and largely rewritten) from BeSpin style
// Copyright (C) 2007 Thomas Luebking <thomas.luebking@web.de>
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

#include <config.h>
#include "blurhelper.h"

#include <QEvent>
#include <QVector>
#include <QDockWidget>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QToolBar>

#include <qtcurve-utils/qtutils.h>
#include <qtcurve-utils/x11blur.h>

namespace QtCurve {
BlurHelper::BlurHelper(QObject *parent):
    QObject(parent),
    _enabled(false)
{
}

void
BlurHelper::registerWidget(QWidget* widget)
{
    widget->installEventFilter(this);
}

void
BlurHelper::unregisterWidget(QWidget *widget)
{
    widget->removeEventFilter(this);
    if (isTransparent(widget)) {
        clear(qtcGetWid(widget));
    }
}

bool
BlurHelper::eventFilter(QObject *object, QEvent *event)
{
    // do nothing if not enabled
    if (!enabled())
        return false;
    switch (event->type()) {
    case QEvent::Hide: {
        QWidget *widget = qtcToWidget(object);
        if (widget && isOpaque(widget)) {
            QWidget *window(widget->window());
            if (window && isTransparent(window) &&
                !_pendingWidgets.contains(window)) {
                _pendingWidgets.insert(window, window);
                delayedUpdate();
            }
        }
        break;
    }
    case QEvent::Show:
    case QEvent::Resize: {
        // cast to widget and check
        QWidget *widget = qtcToWidget(object);
        if (!widget)
            break;
        if (isTransparent(widget)) {
            _pendingWidgets.insert(widget, widget);
            delayedUpdate();
        } else if (isOpaque(widget)) {
            QWidget* window(widget->window());
            if (isTransparent(window)) {
                _pendingWidgets.insert(window, window);
                delayedUpdate();
            }
        }
        break;
    }
    default:
        break;
    }
    // never eat events
    return false;
}

QRegion
BlurHelper::blurRegion(QWidget *widget) const
{
    if (!widget->isVisible())
        return QRegion();
    // get main region
    QRegion region = widget->mask().isEmpty() ? widget->rect() : widget->mask();
    // trim blur region to remove unnecessary areas
    trimBlurRegion(widget, widget, region);
    return region;
}

void
BlurHelper::trimBlurRegion(QWidget *parent, QWidget *widget,
                           QRegion &region) const
{
    // TODO:
    //     Maybe we should clip children with parent? In case we hit this[1] kind
    //     of bugs again.
    //     [1] https://bugs.kde.org/show_bug.cgi?id=306631
    // loop over children
    foreach (QObject *childObject, widget->children()) {
        QWidget *child = qtcToWidget(childObject);
        if (!(child && child->isVisible()))
            continue;
        if (isOpaque(child)) {
            const QPoint offset(child->mapTo(parent, QPoint(0, 0)));
            if (child->mask().isEmpty()) {
                region -= child->rect().translated(offset);
            } else {
                region -= child->mask().translated(offset);
            }
        } else {
            trimBlurRegion(parent, child, region);
        }
    }
    return;
}

void
BlurHelper::update(QWidget *widget) const
{
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    QTC_RET_IF_FAIL(qtcX11Enabled());
    // Do not create native window if there isn't one yet.
    WId wid = qtcGetWid(widget);
    if (!wid) {
        return;
    }
    const QRegion region(blurRegion(widget));
    if (region.isEmpty()) {
        clear(wid);
    } else {
        QVector<uint32_t> data;
        foreach (const QRect &rect, region.rects()) {
            data << rect.x() << rect.y() << rect.width() << rect.height();
        }
        qtcX11BlurTrigger(wid, true, data.size(), data.constData());
    }
    // force update
    if (widget->isVisible()) {
        widget->update();
    }
}

void
BlurHelper::clear(WId wid) const
{
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    QTC_RET_IF_FAIL(qtcX11Enabled());
    qtcX11BlurTrigger(wid, false, 0, 0);
}
}
