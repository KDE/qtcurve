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

#include "config.h"
#include <qtcurve-utils/x11utils.h>
#include <qtcurve-utils/log.h>

#include "utils.h"
#include <QDir>

#ifdef Q_WS_X11
#  include <QX11Info>
#endif

#ifndef QTC_QT4_ENABLE_KDE
#  undef KDE_IS_VERSION
#  define KDE_IS_VERSION(A, B, C) 0
#else
#  include <kdeversion.h>
#  include <KDE/KWindowSystem>
#endif

namespace QtCurve {
namespace Utils {

bool
compositingActive()
{
#if !defined QTC_QT4_ENABLE_KDE || !KDE_IS_VERSION(4, 4, 0)
    return qtcX11CompositingActive();
#else // QTC_QT4_ENABLE_KDE
    return KWindowSystem::compositingActive();
#endif // QTC_QT4_ENABLE_KDE
}

bool
hasAlphaChannel(const QWidget *widget)
{
    QTC_RET_IF_FAIL(widget, false);
#ifdef Q_WS_X11
    return widget->x11Info().depth() == 32;
#else
    return compositingActive();
#endif
}

QString kdeHome()
{
    static QString kdeHomePath;
    if (kdeHomePath.isEmpty()) {
        kdeHomePath = QString::fromLocal8Bit(qgetenv("KDEHOME"));
        if (kdeHomePath.isEmpty()) {
            const QString homePath = QDir::homePath();
            const QDir homeDir = QDir(homePath);
            if (homeDir.exists(QLatin1String(".kde4"))) {
                kdeHomePath = homePath + "/.kde4";
            } else {
                kdeHomePath = homePath + "/.kde";
            }
        }
    }
    return kdeHomePath;
}

}
}
