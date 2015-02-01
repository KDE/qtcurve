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

#ifndef blurhelper_h
#define blurhelper_h

//////////////////////////////////////////////////////////////////////////////
// blurhelper.h
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

#include <qtcurve-utils/log.h>
#include "utils.h"

#include <QPointer>
#include <QHash>
#include <QBasicTimer>
#include <QTimerEvent>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QRegion>
#include <QToolBar>

namespace QtCurve {
class BlurHelper: public QObject {
    Q_OBJECT
public:
    //! constructor
    BlurHelper(QObject*);

    //! enable state
    void setEnabled(bool value)
    {
        _enabled = value;
    }

    //! enabled
    bool enabled() const
    {
        return _enabled;
    }

    //! register widget
    void registerWidget(QWidget*);

    //! register widget
    void unregisterWidget(QWidget*);

    //! event filter
    bool eventFilter(QObject*, QEvent*) override;

protected:
    //! timer event
    /*! used to perform delayed blur region update of pending widgets */
    void timerEvent(QTimerEvent *event) override
    {
        if (event->timerId() == _timer.timerId()) {
            _timer.stop();
            update();
        } else {
            QObject::timerEvent(event);
        }
    }

    //! get list of blur-behind regions matching a given widget
    QRegion blurRegion(QWidget*) const;

    //! trim blur region to remove unnecessary areas (recursive)
    void trimBlurRegion(QWidget*, QWidget*, QRegion&) const;

    //! update blur region for all pending widgets
    /*! a timer is used to allow some buffering of the update requests */
    void delayedUpdate()
    {
        if(!_timer.isActive()) {
            _timer.start(10, this);
        }
    }

    //! update blur region for all pending widgets
    void update() {
        for (const WidgetPointer &widget: const_(_pendingWidgets)) {
            if (widget) {
                update(widget.data());
            }
        }
        _pendingWidgets.clear();
    }

    //! update blur regions for given widget
    void update(QWidget*) const;

    //! clear blur regions for given widget
    void clear(WId) const;

    //! returns true if a given widget is opaque
    bool isOpaque(const QWidget *widget) const
    {
        // TODO:
        // Figure out what is the right thing to do with Qt::WA_OpaquePaintEvent
        return (!widget->isWindow()) &&
            ((widget->autoFillBackground() &&
              widget->palette().color(widget->backgroundRole()).alpha() ==
              0xff) || widget->testAttribute(Qt::WA_OpaquePaintEvent));
    }

    //! true if widget is a transparent window
    /*! some additional checks are performed to make sure stuff like plasma tooltips
      don't get their blur region overwritten */
    inline bool isTransparent(const QWidget* widget) const;

private:
    //! enability
    bool _enabled;

    //! list of widgets for which blur region must be updated
    typedef QPointer<QWidget> WidgetPointer;
    typedef QHash<QWidget*, WidgetPointer> WidgetSet;
    WidgetSet _pendingWidgets;

    //! delayed update timer
    QBasicTimer _timer;
};

bool
BlurHelper::isTransparent(const QWidget *widget) const
{
    return (widget->isWindow() &&
            // widgets using qgraphicsview
            !(widget->graphicsProxyWidget() ||
              widget->inherits("Plasma::Dialog")) &&
            // flags and special widgets
            (widget->testAttribute(Qt::WA_StyledBackground) ||
             qobject_cast<const QMenu*>(widget) ||
             // TODO temporary solution only
             widget->inherits("QComboBoxPrivateContainer") ||
             qobject_cast<const QDockWidget*>(widget) ||
             qobject_cast<const QToolBar*>(widget) ||
             // konsole (thought that should be handled
             // internally by the application
             widget->inherits("Konsole::MainWindow")) &&
            Utils::hasAlphaChannel(widget));
}

}

#endif
