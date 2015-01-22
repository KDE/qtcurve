/*****************************************************************************
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

#include "qtcurve_plugin.h"
#include "qtcurve.h"

#include "config-qt5.h"

#include <qtcurve-utils/qtprops.h>
#include <qtcurve-utils/x11shadow.h>
#include <qtcurve-utils/x11blur.h>

#include <QApplication>

#ifdef Qt5X11Extras_FOUND
#include <qtcurve-utils/x11base.h>
#  include <QX11Info>
#endif

#ifdef QTC_QT5_ENABLE_QTQUICK2
#  include <QQuickWindow>
#  include <QQuickItem>
#endif
#include <QDebug>

namespace QtCurve {

static bool inited = false;

__attribute__((hot)) static void
qtcPolishQuickControl(QObject *obj)
{
#ifdef QTC_QT5_ENABLE_QTQUICK2
    if (QQuickWindow *window = qobject_cast<QQuickWindow*>(obj)) {
        // QtQuickControl support
        // This is still VERY experimental.
        // Need a lot more testing and refactoring.
        if (Style *style = qtcGetStyle(qApp)) {
            if (window->inherits("QQuickPopupWindow")) {
                if (window->inherits("QQuickMenuPopupWindow")) {
                    window->setColor(QColor(0, 0, 0, 0));
                }
                qtcX11ShadowInstall(window->winId());
            } else {
                QColor color = window->color();
                int opacity = style->options().bgndOpacity;
                if (color.alpha() == 255 && opacity != 100) {
                    qreal opacityF = opacity / 100.0;
                    window->setColor(QColor::fromRgbF(color.redF() * opacityF,
                                                      color.greenF() * opacityF,
                                                      color.blueF() * opacityF,
                                                      opacityF));
                    qtcX11BlurTrigger(window->winId(), true, 0, NULL);
                }
            }
        }
    } else if (QQuickItem *item = qobject_cast<QQuickItem*>(obj)) {
        if (QQuickWindow *window = item->window()) {
            if (qtcGetStyle(qApp)) {
                window->setColor(QColor(0, 0, 0, 0));
                qtcX11BlurTrigger(window->winId(), true, 0, NULL);
            }
        }
    }
#else
    QTC_UNUSED(obj);
#endif
}

__attribute__((hot)) static bool
qtcEventCallback(void **cbdata)
{
    QObject *receiver = (QObject*)cbdata[0];
    QTC_RET_IF_FAIL(receiver, false);
    QEvent *event = (QEvent*)cbdata[1];
    if (qtcUnlikely(event->type() == QEvent::DynamicPropertyChange)) {
        QDynamicPropertyChangeEvent *prop_event =
            static_cast<QDynamicPropertyChangeEvent*>(event);
        // eat the property change events from ourselves
        if (prop_event->propertyName() == QTC_PROP_NAME) {
            return true;
        }
    }
    QWidget *widget = qtcToWidget(receiver);
    if (qtcUnlikely(widget && !qtcGetWid(widget))) {
        if (Style *style = qtcGetStyle(widget)) {
            style->prePolish(widget);
        }
    } else if (widget && event->type() == QEvent::UpdateRequest) {
        QtcQWidgetProps props(widget);
        props->opacity = 100;
    } else {
        qtcPolishQuickControl(receiver);
    }
    return false;
}

QStyle*
StylePlugin::create(const QString &key)
{
    init();
    return key.toLower() == "qtcurve" ? new Style : 0;
}

void
StylePlugin::init()
{
    if (inited)
        return;
    inited = true;
    QInternal::registerCallback(QInternal::EventNotifyCallback,
                                qtcEventCallback);
#ifdef QTC_QT5_ENABLE_QTQUICK2
    QQuickWindow::setDefaultAlphaBuffer(true);
#endif
#ifdef Qt5X11Extras_FOUND
    if (qApp->platformName() == "xcb") {
        qtcX11InitXcb(QX11Info::connection(), QX11Info::appScreen());
    }
#endif
}
}
