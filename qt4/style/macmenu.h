/*****************************************************************************
 *   Copyright 2007 Thomas Luebking <thomas.luebking@web.de>                 *
 *   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#ifndef MAC_MENU_H
#define MAC_MENU_H

#include <QMap>
#include <QObject>
#include <QPointer>

class QMenuBar;
class QAction;
class QActionEvent;

namespace Bespin {
class FullscreenWatcher: public QObject {
public:
    FullscreenWatcher(): QObject()
    {
    }
protected:
    bool eventFilter(QObject *o, QEvent *ev);
};

class MacMenu: public QObject {
    Q_OBJECT
public:
    static void manage(QMenuBar *menu);
    static void release(QMenuBar *menu);
    static bool isActive();
    void popup(qlonglong key, int idx, int x, int y);
    void hover(qlonglong key, int idx, int x, int y);
    void popDown(qlonglong key);
    void raise(qlonglong key);

public Q_SLOTS:
    void activate();
    void deactivate();

protected:
    bool eventFilter(QObject *o, QEvent *ev);
    void deactivate(QWidget *window);
    void activate(QWidget *window);
    friend class FullscreenWatcher;

private Q_SLOTS:
    void menuClosed();
    void _release(QObject*);

private:
    Q_DISABLE_COPY(MacMenu)
    MacMenu();
    void activate(QMenuBar *menu);
    void changeAction(QMenuBar *menu, QActionEvent *ev);
    void deactivate(QMenuBar *menu);
    typedef QPointer<QMenuBar> QMenuBar_p;
    typedef QList<QMenuBar_p> MenuList;
    MenuList items;
    QMenuBar *menuBar(qlonglong key);
    QMap<QMenuBar_p, QList<QAction*> > actions;
    bool usingMacMenu;
    QString service;
};
} // namespace

#endif // MAC_MENU_H
