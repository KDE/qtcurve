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

#ifndef shadowhelper_h
#define shadowhelper_h

//////////////////////////////////////////////////////////////////////////////
// shadowhelper.h
// handle shadow pixmaps passed to window manager via X property
// -------------------
//
// Copyright (c) 2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
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

#include <QObject>

namespace QtCurve {
//! handle shadow pixmaps passed to window manager via X property
class ShadowHelper: public QObject {
    Q_OBJECT
public:
    //!@name property names
    static const char *const netWMForceShadowPropertyName;
    static const char *const netWMSkipShadowPropertyName;
    //! constructor
    ShadowHelper(QObject *parent): QObject(parent)
    {
    }

    //! register widget
    bool registerWidget(QWidget*, bool force=false);

    //! unregister widget
    void unregisterWidget(QWidget*);

    //! event filter
    bool eventFilter(QObject*, QEvent*) override;

protected:
    //! accept widget
    bool acceptWidget(QWidget*) const;

    //! install shadow X11 property on given widget
    bool installX11Shadows(QWidget*);

    //! uninstall shadow X11 property on given widget
    void uninstallX11Shadows(QWidget*) const;
};
}

#endif
