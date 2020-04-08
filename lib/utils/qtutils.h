/*****************************************************************************
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

#ifndef __QTC_UTILS_QT_UTILS_H__
#define __QTC_UTILS_QT_UTILS_H__

#include "utils.h"
#include <QtGlobal>
#include <QWidget>
#include <config.h>
#include <QStyleOption>
#include <QObject>
#include <QRect>

namespace QtCurve {

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0) && defined QTC_QT4_ENABLE_KDE) || \
    (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) && defined QTC_QT5_ENABLE_KDE)
#define _QTC_QT_ENABLE_KDE 1
#else
#define _QTC_QT_ENABLE_KDE 0
#endif

#define qtcCheckKDEType(obj, type) (qtcCheckKDETypeFull(obj, type, #type))

template<typename T, typename T2>
QTC_ALWAYS_INLINE static inline const T*
qtcObjCast(const T2 *obj)
{
    return obj ? qobject_cast<const T*>(obj) : nullptr;
}
template<typename T, typename T2>
QTC_ALWAYS_INLINE static inline T*
qtcObjCast(T2 *obj)
{
    return obj ? qobject_cast<T*>(obj) : nullptr;
}

template <class T, class T2> static inline bool
qtcCheckType(T2 *obj)
{
    return qtcObjCast<T>(obj);
}
template <class T2> static inline bool
qtcCheckType(T2 *obj, const char *name)
{
    return obj && obj->inherits(name);
}

#if _QTC_QT_ENABLE_KDE
#define qtcCheckKDETypeFull(obj, type, name) (qtcCheckType<type>(obj))
#else
#define qtcCheckKDETypeFull(obj, type, name) (qtcCheckType(obj, name))
#endif

QTC_ALWAYS_INLINE static inline WId
qtcGetWid(const QWidget *w)
{
    if (!(w && w->testAttribute(Qt::WA_WState_Created))) {
        return (WId)0;
    }
    return w->internalWinId();
}

QTC_ALWAYS_INLINE static inline Qt::WindowType
qtcGetWindowType(const QWidget *w)
{
    return w ? w->windowType() : (Qt::WindowType)0;
}

QTC_ALWAYS_INLINE static inline bool
qtcIsDialog(const QWidget *w)
{
    return oneOf(qtcGetWindowType(w), Qt::Dialog, Qt::Sheet);
}

QTC_ALWAYS_INLINE static inline bool
qtcIsWindow(const QWidget *w)
{
    return oneOf(qtcGetWindowType(w), Qt::Window);
}

QTC_ALWAYS_INLINE static inline bool
qtcIsToolTip(const QWidget *w)
{
    return oneOf(qtcGetWindowType(w), Qt::Tool, Qt::SplashScreen,
                 Qt::ToolTip, Qt::Drawer);
}

QTC_ALWAYS_INLINE static inline bool
qtcIsPopup(const QWidget *w)
{
    return oneOf(qtcGetWindowType(w), Qt::Popup);
}

QTC_ALWAYS_INLINE static inline QWidget*
qtcToWidget(QObject *obj)
{
    if (obj->isWidgetType()) {
        return static_cast<QWidget*>(obj);
    }
    return nullptr;
}

QTC_ALWAYS_INLINE static inline const QWidget*
qtcToWidget(const QObject *obj)
{
    if (obj->isWidgetType()) {
        return static_cast<const QWidget*>(obj);
    }
    return nullptr;
}

class Style;
template<class StyleType=Style, class C>
static inline StyleType*
getStyle(const C *obj)
{
    QStyle *style = obj->style();
    return style ? qobject_cast<StyleType*>(style) : 0;
}

// Get the width and direction of the arrow in a QBalloonTip
QTC_ALWAYS_INLINE static inline int
qtcGetBalloonMargin(QWidget *widget, bool *atTop)
{
    auto margins = widget->contentsMargins();
    int topMargin = margins.top();
    int bottomMargin = margins.bottom();
    if (topMargin > bottomMargin) {
        *atTop = true;
        return topMargin - bottomMargin;
    } else {
        *atTop = false;
        return bottomMargin - topMargin;
    }
}

template<typename T, typename T2>
QTC_ALWAYS_INLINE static inline const T*
styleOptCast(const T2 *opt)
{
    return qstyleoption_cast<const T*>(opt);
}

template<typename T, typename T2>
QTC_ALWAYS_INLINE static inline T*
styleOptCast(T2 *opt)
{
    return qstyleoption_cast<T*>(opt);
}

template<unsigned level=1>
QTC_ALWAYS_INLINE static inline QWidget*
getParent(const QWidget *w)
{
    if (const QWidget *parent = getParent<level - 1>(w)) {
        return parent->parentWidget();
    }
    return nullptr;
}

template<>
inline QWidget*
getParent<0>(const QWidget *w)
{
    return const_cast<QWidget*>(w);
}

template<unsigned level=1>
QTC_ALWAYS_INLINE static inline QObject*
getParent(const QObject *o)
{
    if (const QObject *parent = getParent<level - 1>(o)) {
        return parent->parent();
    }
    return nullptr;
}

template<>
inline QObject*
getParent<0>(const QObject *o)
{
    return const_cast<QObject*>(o);
}

#define _qtcSlotWithTypes(self, name, types)                    \
    self, static_cast<void(qtcPtrType<decltype(self)>::*)types> \
        (qtcMemPtr(self, name))

#define _qtcSlotWithoutTypes(self, name)        \
    self, qtcMemPtr(self, name)

#define qtcSlot(self, name, types...)                           \
    QTC_SWITCH(types, _qtcSlotWithTypes, _qtcSlotWithoutTypes)  \
    (self, name, ##types)

static inline QRect centerRect(const QRect &rect, int width, int height)
{
    return QRect(rect.left() + (rect.width() - width) / 2,
                 rect.top() + (rect.height() - height) / 2, width, height);
}

static inline QRect centerRect(const QRect &rect, const QSize &size)
{
    return centerRect(rect, size.width(), size.height());
}

}

#endif
