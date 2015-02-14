/*****************************************************************************
 *   Copyright 2007 Thomas Luebking <thomas.luebking@web.de>                 *
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

#ifndef MAC_MENU_ADAPTOR_H
#define MAC_MENU_ADAPTOR_H

#include <QDBusAbstractAdaptor>
#include "macmenu.h"

namespace Bespin {
class MacMenuAdaptor: public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.XBarClient")

private:
    MacMenu *mm;

public:
    MacMenuAdaptor(MacMenu *macMenu): QDBusAbstractAdaptor(macMenu),
        mm(macMenu)
    {
    }

public Q_SLOTS:
    Q_NOREPLY void
    activate()
    {
        mm->activate();
    }
    Q_NOREPLY void
    deactivate()
    {
        mm->deactivate();
    }
    Q_NOREPLY void
    popup(qlonglong key, int idx, int x, int y)
    {
        mm->popup(key, idx, x, y);
    }
    Q_NOREPLY void
    hover(qlonglong key, int idx, int x, int y)
    {
        mm->hover(key, idx, x, y);
    }
    Q_NOREPLY void
    popDown(qlonglong key)
    {
        mm->popDown(key);
    }
    Q_NOREPLY void
    raise(qlonglong key)
    {
        mm->raise(key);
    }
};
} // namespace

#endif // MAC_MENU_ADAPTOR_H
