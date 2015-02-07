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
#include "argbhelper.h"
#include <QDesktopWidget>
#include <QApplication>

// Copied from qt_x11_p.h.
// This is not part of the public API but should be stable enough to use
// because it had never changed since the first git commit of Qt.
struct QX11InfoData {
    uint ref;
    int screen;
    int dpiX;
    int dpiY;
    int depth;
    int cells;
    unsigned long colormap;
    void *visual;
    bool defaultColormap;
    bool defaultVisual;
    int subpixel;
};

namespace QtCurve {
bool QtcX11Info::creatingDummy = false;

#ifdef Q_WS_X11
inline QtcX11Info*
QtcX11Info::getInfo(const QWidget *w)
{
    return static_cast<QtcX11Info*>(const_cast<QX11Info*>(&w->x11Info()));
}

// Qt uses XCreateSimpleWindow when defaultVisual and defaultColormap
// are true. This confuses QGLWidget when recreating window caused by
// reparenting to a widget with different depth, result in a mismatch
// in x11info and native window.
inline void
QtcX11Info::fixVisual()
{
    if (qtcUnlikely(!x11data))
        setX11Data(getX11Data(true));
    x11data->defaultVisual = false;
    x11data->defaultColormap = false;
}

inline void
QtcX11Info::setRgba()
{
#if 1
    setX11Data(getInfo(rgbaDummy())->x11data);
#else
    // It seems that VirtualBox is doing something special to Qt so that
    // a TranslucentBackground Widget will NOT have a related 32bit window.
    // The following code enables translucent background in Virtualbox by
    // setting rgba visual on the widget anyway. However, this breaks
    // the display of the virtual machine. Since Gammaray does not work either
    // on VirtualBox, it is hard to figure out what is wrong and which are
    // the widgets need to be blacklist. Disable the code for now unless a
    // workaround is found.
    QX11InfoData *xd = getX11Data(true);
    xd->visual = qtcX11RgbaVisual(&xd->colormap, &xd->cells, xd->screen);
    xd->depth = 32;
    xd->defaultVisual = false;
    xd->defaultColormap = false;
    setX11Data(xd);
#endif
}

inline QWidget*
QtcX11Info::rgbaDummy()
{
    static QWidget **dummies = NULL;
    int scrno = screen();
    if (qtcUnlikely(!dummies || !dummies[scrno])) {
        creatingDummy = true;
        QDesktopWidget *desktop = qApp->desktop();
        if (qtcUnlikely(!dummies))
            dummies = qtcNew(QWidget*, desktop->screenCount());
        dummies[scrno] = new QWidget(desktop->screen(scrno));
        dummies[scrno]->setAttribute(Qt::WA_TranslucentBackground);
        dummies[scrno]->setAttribute(Qt::WA_WState_Polished);
        dummies[scrno]->winId();
        creatingDummy = false;
    }
    return dummies[scrno];
}

void
fixVisual(QWidget *widget)
{
    // Don't use XCreateSimpleWindow
    QtcX11Info::getInfo(widget)->fixVisual();
}

void
addAlphaChannel(QWidget *widget)
{
    QtcX11Info::getInfo(widget)->setRgba();
}
#else
void
fixVisual(QWidget*)
{
}

void
addAlphaChannel(QWidget*)
{
}
#endif

}
