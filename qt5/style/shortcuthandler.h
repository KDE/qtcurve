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

#ifndef __SHORTCUT_HANDLER_H__
#define __SHORTCUT_HANDLER_H__

#include <QObject>
#include <QSet>
#include <QList>

class QWidget;

namespace QtCurve {

class ShortcutHandler: public QObject {
    Q_OBJECT
public:
    explicit ShortcutHandler(QObject *parent = 0);
    ~ShortcutHandler() override;

    bool hasSeenAlt(const QWidget *widget) const;
    bool isAltDown() const { return m_altDown; }
    bool showShortcut(const QWidget *widget) const;

protected:
    void updateWidget(QWidget *w);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    bool m_altDown;
    QSet<QWidget*> m_seenAlt;
    QSet<QWidget*> m_updated;
    QList<QWidget*> m_openMenus;

    void widgetDestroyed(QObject *o);
};

}

#endif
