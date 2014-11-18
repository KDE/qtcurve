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
// qtcurvesizegrip.cpp
// -------------------
//
// Taken from Oxygen kwin decoration
// ------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
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

#include <qtcurve-utils/x11wrap.h>
#include <qtcurve-utils/x11utils.h>

#include "qtcurvesizegrip.h"
#include "qtcurvebutton.h"
#include "qtcurveclient.h"

#include <cassert>
#include <QPainter>
#include <QPolygon>
#include <QTimer>

namespace KWinQtCurve {
static inline bool
similar(const QColor &a, const QColor &b)
{
    static const int diff = 18;
    return (abs(a.red() - b.red()) < diff &&
            abs(a.green() - b.green()) < diff &&
            abs(a.blue() - b.blue()) < diff);
}

QtCurveSizeGrip::QtCurveSizeGrip(QtCurveClient* client):
    QWidget(0),
    client_(client)
{
    setAttribute(Qt::WA_NoSystemBackground );
    setAutoFillBackground(false);

    // cursor
    setCursor(Qt::SizeFDiagCursor);

    // size
    setFixedSize(QSize(GRIP_SIZE, GRIP_SIZE));

    // mask
    QPolygon p;
    p << QPoint(0, GRIP_SIZE)
      << QPoint(GRIP_SIZE, 0)
      << QPoint(GRIP_SIZE, GRIP_SIZE)
      << QPoint(0, GRIP_SIZE);

    setMask(QRegion(p));

    // embed
    embed();
    updatePosition();

    // event filter
    client->widget()->installEventFilter(this);

    // show
    show();
}

QtCurveSizeGrip::~QtCurveSizeGrip()
{
}

void
QtCurveSizeGrip::activeChange()
{
    qtcX11MapRaised(winId());
}

void
QtCurveSizeGrip::embed()
{
    WId window_id = client().windowId();
    if (client().isPreview()) {
        setParent(client().widget());
    } else if (window_id) {
        WId current = window_id;
        while (true) {
            auto reply = qtcX11QueryTree(current);
            if (reply && reply->parent && reply->parent != reply->root &&
                reply->parent != current) {
                current = reply->parent;
            } else {
                qtcFree(reply);
                break;
            }
            free(reply);
        }
        qtcX11ReparentWindow(winId(), current, 0, 0);
        qtcX11Flush();
    } else {
        hide();
    }
}

bool
QtCurveSizeGrip::eventFilter(QObject *object, QEvent *event)
{

    if (object != client().widget())
        return false;
    if (event->type() == QEvent::Resize)
        updatePosition();
    return false;
}

void
QtCurveSizeGrip::paintEvent(QPaintEvent*)
{
    // get relevant colors
    QColor base(KDecoration::options()->color(KDecoration::ColorTitleBar,
                                              client().isActive()));
    // QColor light(client().helper().calcDarkColor(base));
    // QColor dark(client().helper().calcDarkColor(base.darker(150)));

    if (similar(base, client().widget()->palette().color(backgroundRole())))
        base = base.value() > 100 ? base.dark(120) : base.light(120);

    // create and configure painter
    QPainter painter(this);
    // painter.setRenderHints(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(base);

    // polygon
    QPolygon p;
    p << QPoint(0, GRIP_SIZE)
      << QPoint(GRIP_SIZE, 0)
      << QPoint(GRIP_SIZE, GRIP_SIZE)
      << QPoint(0, GRIP_SIZE);
    painter.drawPolygon(p);

    // // diagonal border
    // painter.setBrush(Qt::NoBrush);
    // painter.setPen(QPen(dark, 3));
    // painter.drawLine(QPoint(0, GRIP_SIZE), QPoint(GRIP_SIZE, 0));

    // // side borders
    // painter.setPen(QPen(light, 1.5));
    // painter.drawLine(QPoint(1, GRIP_SIZE), QPoint(GRIP_SIZE, GRIP_SIZE));
    // painter.drawLine(QPoint(GRIP_SIZE, 1), QPoint(GRIP_SIZE, GRIP_SIZE));
    // painter.end();
}

void
QtCurveSizeGrip::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::RightButton:
        hide();
        QTimer::singleShot(5000, this, SLOT(show()));
        break;
    case Qt::MidButton:
        hide();
        break;
    case Qt::LeftButton:
      if (rect().contains(event->pos())) {
          // check client window id
          if (!client().windowId())
              break;
          client().widget()->setFocus();
          if (client().decoration()) {
              client().decoration()->performWindowOperation(
                  KDecorationDefines::ResizeOp);
          }
      }
      break;
    default:
        break;
    }
    return;
}

void
QtCurveSizeGrip::updatePosition()
{
    QPoint position(client().width() - GRIP_SIZE - OFFSET,
                    client().height() - GRIP_SIZE - OFFSET);
#if KDE_IS_VERSION(4, 3, 0)
    if (client().isPreview()) {
      position -= QPoint(
        client().layoutMetric(QtCurveClient::LM_BorderRight) +
        client().layoutMetric(QtCurveClient::LM_OuterPaddingRight),
        client().layoutMetric(QtCurveClient::LM_OuterPaddingBottom) +
        client().layoutMetric(QtCurveClient::LM_BorderBottom));
    } else {
#endif
        position -= QPoint(
            client().layoutMetric(QtCurveClient::LM_BorderRight),
            client().layoutMetric(QtCurveClient::LM_BorderBottom));
#if KDE_IS_VERSION(4, 3, 0)
    }
#endif
    move( position );
}
}
