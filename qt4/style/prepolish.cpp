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
    if (!widget || QtcX11Info::creatingDummy)
        return;

    QtcQWidgetProps props(widget);
    // Don't use XCreateSimpleWindow
    fixVisual(widget);
    // HACK:
    // Modify X11Info of toplevel widgets before they create native windows.
    // This way we won't interfere with widgets that set this property
    // themselves, e.g. plasma, kscreenlock.
    // We do this on windows that are typically shown right after being
    // created before entering the main loop and therefore do not have a
    // chance to be polished before creating window id. In this way, we can
    // avoid recreating native window which breaks a lot of applications.
    // This way should work for all applications except when the application
    // relies on a native RGB window since the children of a RGBA window in
    // Qt are usually also RGBA. The only example of such application I have
    // found so far is kaffeine. See workaround bellow. (NOTE: gl widget works
    // because it is treated differently in Qt) (NOTE2: gl widget will not work
    // straightforwardly when reparenting to a 32bit window due to a bug in
    // Qt4, which causes a 24bit x11info and 32bit gl window to be created
    // The fixVisual() above should work around it, too lazy to
    // report upstream..... :-P).

    // TODO:
    //     use all information to check if a widget should be transparent.
    //     Maybe we can also do sth to their parents' and/or children as well
    if (!widget->testAttribute(Qt::WA_WState_Polished) &&
        !(widget->windowFlags() & Qt::MSWindowsOwnDC) &&
        !qtcGetWid(widget) && !props->prePolished) {
        // Skip MSWindowsOwnDC since it is set for QGLWidget and not likely to
        // be used in other cases.

        // Fix for kaffeine. Kaffeine needs a RGB window for the XV extension.
        // Setting parent to NULL forces a native RGB window to be created
        // for MediaWidget so that its children will also have RGB visual.
        // Kaffeine later sets the parent again (when adding the to layout)
        // after the native RGB children has already been created and in this
        // case, Qt does not create recreate the children window. This seems to
        // be the only way in Qt4 to have a RGB non-OpenGL window in a RGBA
        // window.
        // TODO: maybe we can hack around parent x11info
        if (opts.bgndOpacity != 100 && widget->inherits("MediaWidget")) {
            widget->setAttribute(Qt::WA_DontCreateNativeAncestors);
            widget->setAttribute(Qt::WA_NativeWindow);
            if (!qtcGetWid(widget)) {
                props->prePolished = true;
                // Kaffeine set parent back after children window has been
                // created.
                widget->setParent(NULL);
                widget->createWinId();
            }
            return;
        }
        // the result of qobject_cast may change if we are called in
        // constructor (which is usually the case we want here) so we only
        // set the prePolished property if we have done something.
        if ((opts.bgndOpacity != 100 && (qtcIsWindow(widget) ||
                                         qtcIsToolTip(widget))) ||
            (opts.dlgOpacity != 100 && qtcIsDialog(widget)) ||
            (opts.menuBgndOpacity != 100 &&
             (qobject_cast<QMenu*>(widget) ||
              widget->inherits("QComboBoxPrivateContainer")))) {
            props->prePolished = true;
            addAlphaChannel(widget);
            // Set this for better efficiency for now
            widget->setAutoFillBackground(false);
        }
    }
}

}
