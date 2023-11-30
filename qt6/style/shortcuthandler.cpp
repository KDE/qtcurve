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

#include "shortcuthandler.h"

#include <qtcurve-utils/utils.h>
#include <qtcurve-utils/qtutils.h>

#include <QWidget>
#include <QMenu>
#include <QMenuBar>
#include <QEvent>
#include <QKeyEvent>

namespace QtCurve {

ShortcutHandler::ShortcutHandler(QObject *parent) :
    QObject(parent), m_altDown(false)
{
}

ShortcutHandler::~ShortcutHandler()
{
}

bool ShortcutHandler::hasSeenAlt(const QWidget *widget) const
{
    if(widget && !widget->isEnabled())
        return false;

    if(qobject_cast<const QMenu*>(widget))
        return m_openMenus.count() && m_openMenus.last()==widget;
//     {
//         const QWidget *w=widget;
//
//         while(w)
//         {
//             if(m_seenAlt.contains((QWidget *)w))
//                 return true;
//             w=w->parentWidget();
//         }
//     }
    else
        return m_openMenus.isEmpty() && m_seenAlt.contains((QWidget*)(widget->window()));

    return false;
}

bool ShortcutHandler::showShortcut(const QWidget *widget) const
{
    return m_altDown && hasSeenAlt(widget);
}

void ShortcutHandler::widgetDestroyed(QObject *o)
{
    m_updated.remove(static_cast<QWidget *>(o));
    m_openMenus.removeAll(static_cast<QWidget *>(o));
}

void ShortcutHandler::updateWidget(QWidget *w)
{
    if (!m_updated.contains(w)) {
        m_updated.insert(w);
        w->update();
        connect(qtcSlot(w, destroyed),
                qtcSlot(this, widgetDestroyed));
    }
}

bool ShortcutHandler::eventFilter(QObject *o, QEvent *e)
{
    if (!o->isWidgetType())
        return QObject::eventFilter(o, e);

    QWidget *widget = static_cast<QWidget*>(o);
    switch(e->type())
    {
    case QEvent::KeyPress:
        if (Qt::Key_Alt==static_cast<QKeyEvent *>(e)->key())
        {
            m_altDown = true;
            if(qobject_cast<QMenu *>(widget))
            {
                m_seenAlt.insert(widget);
                updateWidget(widget);
                if(widget->parentWidget() && widget->parentWidget()->window())
                    m_seenAlt.insert(widget->parentWidget()->window());
            }
            else
            {
                widget = widget->window();
                m_seenAlt.insert(widget);
                QList<QWidget*> l = widget->findChildren<QWidget*>();
                for (int pos = 0;pos < l.size();++pos) {
                    QWidget *w = l.at(pos);
                    if (!(w->isWindow() || !w->isVisible())) // || w->style()->styleHint(QStyle::SH_UnderlineShortcut, 0, w)))
                        updateWidget(w);
                }

                QList<QMenuBar*> m = widget->findChildren<QMenuBar*>();
                for (int i = 0;i < m.size();++i) {
                    updateWidget(m.at(i));
                }
            }
        }
        break;
    case QEvent::WindowDeactivate:
    case QEvent::KeyRelease:
        if (QEvent::WindowDeactivate == e->type() ||
            static_cast<QKeyEvent*>(e)->key() == Qt::Key_Alt) {
            m_altDown = false;
            for (QWidget *widget: const_(m_updated)) {
                widget->update();
            }
            if (!m_updated.contains(widget))
                widget->update();
            m_seenAlt.clear();
            m_updated.clear();
        }
        break;
    case QEvent::Show:
        if(qobject_cast<QMenu *>(widget))
        {
            QWidget *prev=m_openMenus.count() ? m_openMenus.last() : 0L;
            m_openMenus.append(widget);
            if(m_altDown && prev)
                prev->update();
            connect(qtcSlot(widget, destroyed),
                    qtcSlot(this, widgetDestroyed));
        }
        break;
    case QEvent::Hide:
        if(qobject_cast<QMenu *>(widget))
        {
            m_seenAlt.remove(widget);
            m_updated.remove(widget);
            m_openMenus.removeAll(widget);
            if(m_altDown)
            {
                if(m_openMenus.count())
                    m_openMenus.last()->update();
                else if(widget->parentWidget() && widget->parentWidget()->window())
                    widget->parentWidget()->window()->update();
            }
        }
        break;
    case QEvent::Close:
        // Reset widget when closing
        m_seenAlt.remove(widget);
        m_updated.remove(widget);
        m_seenAlt.remove(widget->window());
        m_openMenus.removeAll(widget);
        if(m_altDown)
        {
            if(m_openMenus.count())
                m_openMenus.last()->update();
            else if(widget->parentWidget() && widget->parentWidget()->window())
                widget->parentWidget()->window()->update();
        }
        break;
    default:
        break;
    }
    return QObject::eventFilter(o, e);
}

}
