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

#ifndef __QTC_UTILS_GTK_UTILS_H__
#define __QTC_UTILS_GTK_UTILS_H__

#include "utils.h"
#include <gtk/gtk.h>

namespace QtCurve {
namespace Widget {

static inline cairo_rectangle_int_t
getAllocation(GtkWidget *widget)
{
    cairo_rectangle_int_t alloc;
    // Binary compatible
    gtk_widget_get_allocation(widget, (GdkRectangle*)&alloc);
    return alloc;
}

static inline GtkRequisition
getRequisition(GtkWidget *widget)
{
    GtkRequisition req;
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_get_preferred_size(widget, &req, nullptr);
#else
    gtk_widget_get_requisition(widget, &req);
#endif
    return req;
}

static inline GtkOrientation
getOrientation(GtkWidget *widget)
{
    return gtk_orientable_get_orientation(GTK_ORIENTABLE(widget));
}

static inline bool
isHorizontal(GtkWidget *widget)
{
    return getOrientation(widget) == GTK_ORIENTATION_HORIZONTAL;
}

}

static inline float
qtcFrameGetLabelYAlign(GtkFrame *f)
{
    float x, y;
    gtk_frame_get_label_align(f, &x, &y);
    return y;
}

static inline bool
qtcIsProgressBar(GtkWidget *w)
{
#if GTK_CHECK_VERSION(3, 0, 0)
    if (!GTK_IS_PROGRESS_BAR(w)) {
        return false;
    }
#else
    if (!GTK_IS_PROGRESS(w)) {
        return false;
    }
#endif
    GtkAllocation alloc;
    gtk_widget_get_allocation(w, &alloc);
    return alloc.x != -1 && alloc.y != -1;
}

template<typename T>
static inline const char*
gTypeName(T *obj)
{
    if (qtcUnlikely(!obj))
        return "";
    return qtcDefault(G_OBJECT_TYPE_NAME(obj), "");
}

struct GObjectDeleter {
    template<typename T>
    inline void
    operator()(T *p)
    {
        g_object_unref(p);
    }
    template<typename T>
    inline void
    ref(T *p)
    {
        g_object_ref_sink(p);
    }
};

template<typename T, typename D>
class RefPtr : public std::unique_ptr<T, D> {
public:
    constexpr
    RefPtr()
        : std::unique_ptr<T, D>()
    {
    }
    RefPtr(T *p)
        : std::unique_ptr<T, D>(p)
    {
        if (p) {
            this->get_deleter().ref(p);
        }
    }
    RefPtr(const RefPtr &other)
        : RefPtr(other.get())
    {
    }
    RefPtr(RefPtr &&other)
        : std::unique_ptr<T, D>(std::move(other))
    {
    }
    RefPtr&
    operator=(RefPtr &&other)
    {
        std::unique_ptr<T, D>::operator=(std::move(other));
        return *this;
    }
};

template<typename ObjType=GObject>
using GObjPtr = RefPtr<ObjType, GObjectDeleter>;

}

#endif
