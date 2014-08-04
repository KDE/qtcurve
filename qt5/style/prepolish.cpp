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

#include "config.h"
#include <qtcurve-utils/qtprops.h>

#include "qtcurve_p.h"
#include "argbhelper.h"
#include <QMenu>

namespace QtCurve {

__attribute__((hot)) void
Style::prePolish(QWidget *widget) const
{
    if (!widget)
        return;
    QtcQWidgetProps props(widget);
    // HACK:
    // Request for RGBA format on toplevel widgets before they
    // create native windows. These windows are typically shown after being
    // created before entering the main loop and therefore do not have a
    // chance to be polished before creating window id. (NOTE: somehow the popup
    // menu on mdi sub window in QtDesigner has the same problem).
    // TODO:
    //     Use all information to check if a widget should be transparent.
    //     Need to figure out how Qt5's xcb backend deal with RGB native window
    //     as a child of a RGBA window. However, since Qt5 will not recreate
    //     native window, this is probably easier to deal with than Qt4.
    //     (After we create a RGB window, Qt5 will not override it).
    if (!(widget->windowFlags() & Qt::MSWindowsOwnDC) &&
        !qtcGetWid(widget) && !props->prePolishing) {
        // Skip MSWindowsOwnDC since it is set for QGLWidget and not likely to
        // be used in other cases.
        if ((opts.bgndOpacity != 100 && (qtcIsWindow(widget) ||
                                         qtcIsToolTip(widget))) ||
            (opts.dlgOpacity != 100 && qtcIsDialog(widget)) ||
            (opts.menuBgndOpacity != 100 &&
             (qobject_cast<QMenu*>(widget) ||
              widget->inherits("QComboBoxPrivateContainer")))) {
            props->prePolishing = true;
            addAlphaChannel(widget);
            // QWidgetPrivate::updateIsTranslucent sets the format back
            // is Qt::WA_TranslucentBackground is not set. So we need to do
            // this repeatedly
            props->prePolishing = false;
        }
    }
}
}
