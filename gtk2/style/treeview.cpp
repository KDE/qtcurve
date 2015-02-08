/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#include "treeview.h"

#include <qtcurve-utils/gtkprops.h>
#include <qtcurve-cairo/utils.h>

namespace QtCurve {
namespace TreeView {

typedef struct {
    GtkTreePath *path;
    GtkTreeViewColumn *column;
    bool fullWidth;
} QtCTreeView;

static GHashTable *table = nullptr;

static QtCTreeView*
lookupHash(void *hash, bool create)
{
    QtCTreeView *rv = nullptr;

    if (!table)
        table=g_hash_table_new(g_direct_hash, g_direct_equal);

    rv = (QtCTreeView*)g_hash_table_lookup(table, hash);

    if (!rv && create) {
        rv = qtcNew(QtCTreeView);
        rv->path=nullptr;
        rv->column=nullptr;
        rv->fullWidth=false;
        g_hash_table_insert(table, hash, rv);
        rv = (QtCTreeView*)g_hash_table_lookup(table, hash);
    }

    return rv;
}

static void
removeFromHash(void *hash)
{
    if (table) {
        QtCTreeView *tv = lookupHash(hash, false);
        if (tv) {
            if(tv->path)
                gtk_tree_path_free(tv->path);
            g_hash_table_remove(table, hash);
        }
    }
}

static void
cleanup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && qtcWidgetProps(props)->treeViewHacked) {
        removeFromHash(widget);
        qtcDisconnectFromProp(props, treeViewDestroy);
        qtcDisconnectFromProp(props, treeViewUnrealize);
        qtcDisconnectFromProp(props, treeViewStyleSet);
        qtcDisconnectFromProp(props, treeViewMotion);
        qtcDisconnectFromProp(props, treeViewLeave);
        qtcWidgetProps(props)->treeViewHacked = false;
    }
}

static gboolean
styleSet(GtkWidget *widget, GtkStyle*, void*)
{
    cleanup(widget);
    return false;
}

static gboolean
destroy(GtkWidget *widget, GdkEvent*, void*)
{
    cleanup(widget);
    return false;
}

static bool
samePath(GtkTreePath *a, GtkTreePath *b)
{
    return a ? (b && !gtk_tree_path_compare(a, b)) : !b;
}

static void
updatePosition(GtkWidget *widget, int x, int y)
{
    if (GTK_IS_TREE_VIEW(widget)) {
        QtCTreeView *tv = lookupHash(widget, false);
        if (tv) {
            GtkTreeView *treeView = GTK_TREE_VIEW(widget);
            GtkTreePath *path = nullptr;
            GtkTreeViewColumn *column = nullptr;

            gtk_tree_view_get_path_at_pos(treeView, x, y, &path,
                                          &column, 0L, 0L);

            if (!samePath(tv->path, path)) {
                // prepare update area
                // get old rectangle
                QtcRect oldRect = {0, 0, -1, -1 };
                QtcRect newRect = {0, 0, -1, -1 };
                QtcRect updateRect;
                QtcRect alloc = Widget::getAllocation(widget);

                if (tv->path && tv->column) {
                    gtk_tree_view_get_background_area(
                        treeView, tv->path, tv->column,
                        (GdkRectangle*)&oldRect);
                }
                if (tv->fullWidth) {
                    oldRect.x = 0;
                    oldRect.width = alloc.width;
                }

                // get new rectangle and update position
                if (path && column) {
                    gtk_tree_view_get_background_area(
                        treeView, path, column, (GdkRectangle*)&newRect);
                }
                if (path && column && tv->fullWidth) {
                    newRect.x = 0;
                    newRect.width = alloc.width;
                }

                // take the union of both rectangles
                if (oldRect.width > 0 && oldRect.height > 0) {
                    if (newRect.width > 0 && newRect.height > 0) {
                        Rect::union_(&oldRect, &newRect, &updateRect);
                    } else {
                        updateRect = oldRect;
                    }
                } else {
                    updateRect = newRect;
                }

                // store new cell info
                if (tv->path)
                    gtk_tree_path_free(tv->path);
                tv->path = path ? gtk_tree_path_copy(path) : nullptr;
                tv->column = column;

                // convert to widget coordinates and schedule redraw
                gtk_tree_view_convert_bin_window_to_widget_coords(
                    treeView, updateRect.x, updateRect.y,
                    &updateRect.x, &updateRect.y);
                gtk_widget_queue_draw_area(
                    widget, updateRect.x, updateRect.y,
                    updateRect.width, updateRect.height);
            }

            if (path) {
                gtk_tree_path_free(path);
            }
        }
    }
}

static gboolean
motion(GtkWidget *widget, GdkEventMotion *event, void*)
{
    if (event && event->window && GTK_IS_TREE_VIEW(widget) &&
        gtk_tree_view_get_bin_window(GTK_TREE_VIEW(widget)) == event->window) {
        updatePosition(widget, event->x, event->y);
    }
    return false;
}

static gboolean
leave(GtkWidget *widget, GdkEventMotion*, void*)
{
    if (GTK_IS_TREE_VIEW(widget)) {
        QtCTreeView *tv = lookupHash(widget, false);
        if (tv) {
            GtkTreeView *treeView = GTK_TREE_VIEW(widget);
            QtcRect rect = {0, 0, -1, -1 };
            QtcRect alloc = Widget::getAllocation(widget);

            if (tv->path && tv->column) {
                gtk_tree_view_get_background_area(
                    treeView, tv->path, tv->column, (GdkRectangle*)&rect);
            }
            if (tv->fullWidth) {
                rect.x = 0;
                rect.width = alloc.width;
            }
            if (tv->path) {
                gtk_tree_path_free(tv->path);
            }
            tv->path = nullptr;
            tv->column = nullptr;

            gtk_tree_view_convert_bin_window_to_widget_coords(
                treeView, rect.x, rect.y, &rect.x, &rect.y);
            gtk_widget_queue_draw_area(
                widget, rect.x, rect.y, rect.width, rect.height);
        }
    }
    return false;
}

void
getCell(GtkTreeView *treeView, GtkTreePath **path, GtkTreeViewColumn **column,
        int x, int y, int width, int height)
{
    const GdkPoint points[4] = {{x + 1, y + 1}, {x + 1, y + height - 1},
                                {x + width - 1, y + 1},
                                {x + width, y + height - 1}};
    for (int pos = 0;pos < 4 && !(*path);pos++) {
        gtk_tree_view_get_path_at_pos(treeView, points[pos].x,
                                      points[pos].y, path, column, 0L, 0L);
    }
}

void
setup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->treeViewHacked) {
        QtCTreeView *tv = lookupHash(widget, true);
        GtkTreeView *treeView = GTK_TREE_VIEW(widget);
        GtkWidget *parent = gtk_widget_get_parent(widget);

        if (tv) {
            qtcWidgetProps(props)->treeViewHacked = true;
            int x, y;
#if GTK_CHECK_VERSION(2, 90, 0) /* Gtk3:TODO !!! */
            tv->fullWidth = true;
#else
            gtk_widget_style_get(widget, "row_ending_details",
                                 &tv->fullWidth, nullptr);
#endif
            gdk_window_get_pointer(gtk_widget_get_window(widget), &x, &y, 0L);
            gtk_tree_view_convert_widget_to_bin_window_coords(treeView, x, y,
                                                              &x, &y);
            updatePosition(widget, x, y);
            qtcConnectToProp(props, treeViewDestroy, "destroy-event",
                             destroy, nullptr);
            qtcConnectToProp(props, treeViewUnrealize, "unrealize",
                             destroy, nullptr);
            qtcConnectToProp(props, treeViewStyleSet, "style-set",
                             styleSet, nullptr);
            qtcConnectToProp(props, treeViewMotion, "motion-notify-event",
                             motion, nullptr);
            qtcConnectToProp(props, treeViewLeave, "leave-notify-event",
                             leave, nullptr);
        }

        if (!gtk_tree_view_get_show_expanders(treeView))
            gtk_tree_view_set_show_expanders(treeView, true);
        if (gtk_tree_view_get_enable_tree_lines(treeView))
            gtk_tree_view_set_enable_tree_lines(treeView, false);

        if (GTK_IS_SCROLLED_WINDOW(parent) &&
            gtk_scrolled_window_get_shadow_type(GTK_SCROLLED_WINDOW(parent)) !=
            GTK_SHADOW_IN) {
            gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(parent),
                                                GTK_SHADOW_IN);
        }
    }
}

bool
isCellHovered(GtkWidget *widget, GtkTreePath *path, GtkTreeViewColumn *column)
{
    QtCTreeView *tv = lookupHash(widget, false);
    return (tv && (tv->fullWidth || tv->column == column) &&
            samePath(path, tv->path));
}

bool
cellIsLeftOfExpanderColumn(GtkTreeView *treeView, GtkTreeViewColumn *column)
{
    // check expander column
    GtkTreeViewColumn *expanderColumn =
        gtk_tree_view_get_expander_column(treeView);

    if (!expanderColumn || column == expanderColumn) {
        return false;
    } else {
        bool found = false;
        bool isLeft = false;
        // get all columns
        GList *columns = gtk_tree_view_get_columns(treeView);
        for (GList *child = columns;child;child = g_list_next(child)) {
            if (!GTK_IS_TREE_VIEW_COLUMN(child->data)) {
                continue;
            }
            GtkTreeViewColumn *childCol = GTK_TREE_VIEW_COLUMN(child->data);
            if (childCol == expanderColumn) {
                if (found) {
                    isLeft = true;
                }
            } else if (found) {
                break;
            } else if (column == childCol) {
                found = true;
            }
        }
        if (columns) {
            g_list_free(columns);
        }
        return isLeft;
    }
}

}
}
