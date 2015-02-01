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

#ifndef __WINDOW_MANAGER_H__
#define __WINDOW_MANAGER_H__

// Copied from oxygenwindowmanager.h svnversion: 1137195

//////////////////////////////////////////////////////////////////////////////
// windowmanager.h
// pass some window mouse press/release/move event actions to window manager
// -------------------
//
// Copyright (c) 2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
//
// Largely inspired from BeSpin style
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

#include <QEvent>
#include <QBasicTimer>
#include <QSet>
#include <QString>
#include <QPointer>
#include <QWidget>

namespace QtCurve {
class WindowManager: public QObject {
    Q_OBJECT
public:
    explicit WindowManager(QObject*);

    //! initialize
    /*! read relevant options from OxygenStyleConfigData */
    void initialize( int windowDrag, const QStringList &whiteList=QStringList(),
                     const QStringList &blackList=QStringList());

    //! register widget
    void registerWidget(QWidget*);

    //! unregister widget
    void unregisterWidget(QWidget*);

    //! event filter [reimplemented]
    bool eventFilter(QObject*, QEvent*) override;

protected:
    //! timer event,
    /*! used to start drag if button is pressed for a long enough time */
    void timerEvent(QTimerEvent*) override;
    //! mouse press event
    bool mousePressEvent( QObject*, QEvent* );
    //! mouse move event
    bool mouseMoveEvent( QObject*, QEvent* );
    //! mouse release event
    bool mouseReleaseEvent( QObject*, QEvent* );
    //!@name configuration
    //@{
    //! enable state
    bool
    enabled() const
    {
        return _enabled;
    }

    //! enable state
    void
    setEnabled(bool value)
    {
        _enabled = value;
    }

    //! returns true if window manager is used for moving
    bool
    useWMMoveResize() const
    {
        return _useWMMoveResize;
    }

    //! drag mode
    int dragMode() const
    {
        return _dragMode;
    }

    //! drag mode
    void setDragMode(int value)
    {
        _dragMode = value;
    }

    //! drag distance (pixels)
    void setDragDistance(int value)
    {
        _dragDistance = value;
    }

    //! drag delay (msec)
    void setDragDelay(int value)
    {
        _dragDelay = value;
    }

    //! set list of whiteListed widgets
    /*!
      white list is read from options and is used to adjust
      per-app window dragging issues
    */
    void initializeWhiteList(const QStringList &list);

    //! set list of blackListed widgets
    /*!
      black list is read from options and is used to adjust
      per-app window dragging issues
    */
    void initializeBlackList(const QStringList &list);

    //@}

    //! returns true if widget is dragable
    bool isDragable(QWidget*);

    //! returns true if widget is dragable
    bool isBlackListed(QWidget*);

    //! returns true if widget is dragable
    bool isWhiteListed(QWidget*) const;

    //! returns true if drag can be started from current widget
    bool canDrag(QWidget*);

    //! returns true if drag can be started from current widget and position
    /*! child at given position is passed as second argument */
    bool canDrag(QWidget*, QWidget*, const QPoint&);

    //! reset drag
    void resetDrag();

    //! start drag
    void startDrag(QWidget*, const QPoint&);

    //! utility function
    bool isDockWidgetTitle(const QWidget*) const;

    //!@name lock
    //@{

    void
    setLocked(bool value)
    {
        _locked = value;
    }

    //! lock
    bool
    isLocked() const
    {
        return _locked;
    }

    //@}

private:
    //! enability
    bool _enabled;

    //! use WM moveResize
    bool _useWMMoveResize;

    //! drag mode
    int _dragMode;

    //! drag distance
    /*! this is copied from kwin::geometry */
    int _dragDistance;

    //! drag delay
    /*! this is copied from kwin::geometry */
    int _dragDelay;

    //! wrapper for exception id
    class ExceptionId: public QPair<QString, QString> {
    public:
        //! constructor
        ExceptionId(const QString &value)
        {
            const QStringList args(value.split("@"));
            if (args.isEmpty())
                return;
            second = args[0].trimmed();
            if (args.size() > 1) {
                first = args[1].trimmed();
            }
        }

        const QString&
        appName() const
        {
            return first;
        }

        const QString&
        className() const
        {
            return second;
        }
    };

    //! exception set
    typedef QSet<ExceptionId> ExceptionSet;

    //! list of white listed special widgets
    /*!
      it is read from options and is used to adjust
      per-app window dragging issues
    */
    ExceptionSet _whiteList;

    //! list of black listed special widgets
    /*!
      it is read from options and is used to adjust
      per-app window dragging issues
    */
    ExceptionSet _blackList;

    //! drag point
    QPoint _dragPoint;
    QPoint _globalDragPoint;

    //! drag timer
    QBasicTimer _dragTimer;

    //! target being dragged
    /*! QPointer is used in case the target gets deleted while drag
      is in progress */
    QPointer<QWidget> _target;

    //! true if drag is about to start
    bool _dragAboutToStart;

    //! true if drag is in progress
    bool _dragInProgress;

    //! true if drag is locked
    bool _locked;

    //! cursor override
    /*! used to keep track of application cursor being overridden when dragging in non-WM mode */
    bool _cursorOverride;

    //! provide application-wise event filter
    /*!
      it us used to unlock dragging and make sure event look is properly restored
      after a drag has occurred
    */
    class AppEventFilter: public QObject {
    public:
        //! constructor
        AppEventFilter(WindowManager* parent):
            QObject(parent),
            _parent(parent)
        {}

        //! event filter
        bool eventFilter( QObject*, QEvent* ) override;

    protected:
        //! application-wise event.
        /*! needed to catch end of XMoveResize events */
        bool appMouseEvent( QObject*, QEvent* );

    private:
        //! parent
        WindowManager* _parent;
    };

    //! application event filter
    AppEventFilter* _appEventFilter;

    //! allow access of all private members to the app event filter
    friend class AppEventFilter;
};

}

#endif
