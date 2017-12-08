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

#include "qtcurve_plugin.h"
#include "qtcurve.h"

#include "config-qt5.h"

#include <qtcurve-utils/qtprops.h>
#include <qtcurve-utils/x11shadow.h>
#include <qtcurve-utils/x11blur.h>

#include <QApplication>

#ifdef Qt5X11Extras_FOUND
#  include <qtcurve-utils/x11base.h>
#  include <QX11Info>
#endif

#ifdef QTC_QT5_ENABLE_QTQUICK2
#  include <QQuickWindow>
#  include <QQuickItem>
#endif
#include <QDebug>

#include <qtcurve-utils/log.h>

namespace QtCurve {

__attribute__((hot)) static void
polishQuickControl(QObject *obj)
{
#ifdef QTC_QT5_ENABLE_QTQUICK2
    if (QQuickWindow *window = qobject_cast<QQuickWindow*>(obj)) {
        // QtQuickControl support
        // This is still VERY experimental.
        // Need a lot more testing and refactoring.
        if (Style *style = getStyle(qApp)) {
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
                    qtcX11BlurTrigger(window->winId(), true, 0, nullptr);
                }
            }
        }
    } else if (QQuickItem *item = qobject_cast<QQuickItem*>(obj)) {
        if (QQuickWindow *window = item->window()) {
            if (getStyle(qApp)) {
                window->setColor(QColor(0, 0, 0, 0));
                qtcX11BlurTrigger(window->winId(), true, 0, nullptr);
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
        if (Style *style = getStyle(widget)) {
            style->prePolish(widget);
        }
    } else if (widget && event->type() == QEvent::UpdateRequest) {
        QtcQWidgetProps props(widget);
        props->opacity = 100;
    } else {
        polishQuickControl(receiver);
    }
    return false;
}

static StylePlugin *firstPlInstance = nullptr;
static QList<Style*> *styleInstances = nullptr;

QStyle*
StylePlugin::create(const QString &key)
{
    if (!firstPlInstance) {
        firstPlInstance = this;
        styleInstances = &m_styleInstances;
    }

    init();
    Style *qtc;
    if (key.toLower() == "qtcurve") {
        qtc = new Style;
        qtc->m_plugin = this;
        // keep track of all style instances we allocate, for instance
        // for KLineEdit widgets which apparently insist on overriding
        // certain things (cf. KLineEditStyle). We want to be able to
        // delete those instances as properly and as early as
        // possible during the global destruction phase.
        m_styleInstances << qtc;
    } else {
        qtc = nullptr;
    }
    return qtc;
}

void StylePlugin::unregisterCallback()
{
    if (m_eventNotifyCallbackInstalled) {
        qtcInfo("Unregistering the event notify callback (for plugin %p)\n", this);
        QInternal::unregisterCallback(QInternal::EventNotifyCallback,
                                    qtcEventCallback);
        m_eventNotifyCallbackInstalled = false;
    }
}

StylePlugin::~StylePlugin()
{
    qtcInfo("Deleting QtCurve plugin (%p)\n", this);
    if (!m_styleInstances.isEmpty()) {
        qtcWarn("there remain(s) %d Style instance(s)\n", m_styleInstances.count());
        foreach (Style *that, m_styleInstances) {
            // don't let ~Style() touch m_styleInstances from here.
            that->m_plugin = nullptr;
            // each instance should already have disconnected from the D-Bus
            // and disconnected from receiving select signals.
            delete that;
        }
        m_styleInstances.clear();
    }
    if (firstPlInstance == this) {
        firstPlInstance = nullptr;
        styleInstances = nullptr;
    }
}

void
StylePlugin::init()
{
    std::call_once(m_ref_flag, [this] {
            QInternal::registerCallback(QInternal::EventNotifyCallback,
                                        qtcEventCallback);
            m_eventNotifyCallbackInstalled = true;
            if (QCoreApplication::instance()) {
                connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &StylePlugin::unregisterCallback);
            }
#ifdef QTC_QT5_ENABLE_QTQUICK2
            QQuickWindow::setDefaultAlphaBuffer(true);
#endif
#ifdef Qt5X11Extras_FOUND
            if (qApp->platformName() == "xcb") {
                qtcX11InitXcb(QX11Info::connection(), QX11Info::appScreen());
            }
#endif
        });
}

__attribute__((constructor)) int atLibOpen()
{
    qtcDebug("Opening QtCurve\n");
    return 0;
}

__attribute__((destructor)) int atLibClose()
{
    qtcInfo("Closing QtCurve\n");
    if (firstPlInstance) {
        qtcInfo("Plugin instance %p still open with %d open Style instance(s)\n",
            firstPlInstance, styleInstances->count());
    }
    return 0;
}

}
