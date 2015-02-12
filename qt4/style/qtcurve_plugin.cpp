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

#include "qtcurve_plugin.h"
#include "qtcurve.h"
#include "config.h"

#include <qtcurve-utils/qtprops.h>
#include <qtcurve-utils/x11base.h>

#ifdef Q_WS_X11
#  include <QX11Info>
#endif

#include <QTypeInfo>
#include <QFileInfo>

namespace QtCurve {

#ifdef QTC_QT4_STYLE_SUPPORT

#include <QDir>

static void
getStyles(const QString &dir, const char *sub, QSet<QString> &styles)
{
    QDir d(dir + sub);

    if (d.exists()) {
        QStringList filters;

        filters << QString(THEME_PREFIX "*" THEME_SUFFIX);
        d.setNameFilters(filters);

        foreach (const QString &str, d.entryList()) {
            QString style(str.left(str.lastIndexOf(THEME_SUFFIX)));
            if (!styles.contains(style)) {
                styles.insert(style);
            }
        }
    }
}

static void
getStyles(const QString &dir, QSet<QString> &styles)
{
    getStyles(dir, THEME_DIR, styles);
    getStyles(dir, THEME_DIR4, styles);
}
#endif

QStringList
StylePlugin::keys() const
{
    QSet<QString> styles;
    styles.insert("QtCurve");

#ifdef QTC_QT4_STYLE_SUPPORT
    getStyles(Utils::kdeHome(), styles);
    getStyles(KDE_PREFIX(4), styles);
#endif
    return styles.toList();
}

QStyle*
StylePlugin::create(const QString &key)
{
    // init needs to be called before Style is created
    init();
    if (key.toLower() == "qtcurve") {
        return new Style;
    }
#ifdef QTC_QT4_STYLE_SUPPORT
    if (key.indexOf(THEME_PREFIX) == 0) {
        return new Style(key);
    }
#endif
    return 0;
}

static bool inited = false;

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
    if (!widget)
        return false;
    if (qtcUnlikely(!widget->testAttribute(Qt::WA_WState_Polished) &&
                    !qtcGetWid(widget))) {
        if (Style *style = getStyle(widget)) {
            style->prePolish(widget);
        }
    } else if (event->type() == QEvent::UpdateRequest) {
        QtcQWidgetProps(widget)->opacity = 100;
    }
    return false;
}

void
StylePlugin::init()
{
    if (inited)
        return;
    inited = true;
    QInternal::registerCallback(QInternal::EventNotifyCallback,
                                qtcEventCallback);
#ifdef Q_WS_X11
    qtcX11InitXlib(QX11Info::display());
#endif
}

Q_EXPORT_PLUGIN2(Style, StylePlugin)

}
