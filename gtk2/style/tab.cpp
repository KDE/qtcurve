/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#include "tab.h"

#include <qtcurve-utils/gtkprops.h>

typedef struct {
    int id;
    int numRects;
    QtcRect *rects;
} QtCTab;

static GHashTable *qtcTabHashTable = NULL;

static QtCTab*
qtcTabLookupHash(void *hash, gboolean create)
{
    if (!qtcTabHashTable) {
        qtcTabHashTable = g_hash_table_new(g_direct_hash, g_direct_equal);
    }

    QtCTab *rv = (QtCTab*)g_hash_table_lookup(qtcTabHashTable, hash);
    if (!rv && create) {
        rv = qtcNew(QtCTab);
        rv->numRects = gtk_notebook_get_n_pages(GTK_NOTEBOOK(hash));
        rv->id = -1;
        rv->rects = qtcNew(QtcRect, rv->numRects);
        for (int p = 0;p < rv->numRects;p++) {
            rv->rects[p].width = rv->rects[p].height = -1;
        }
        g_hash_table_insert(qtcTabHashTable, hash, rv);
        rv = (QtCTab*)g_hash_table_lookup(qtcTabHashTable, hash);
    }
    return rv;
}

static QtCTab*
qtcWidgetFindTab(GtkWidget *widget)
{
    return GTK_IS_NOTEBOOK(widget) ? qtcTabLookupHash(widget, false) : NULL;
}

static void
qtcTabRemoveHash(void *hash)
{
    if (qtcTabHashTable) {
        QtCTab *tab = qtcWidgetFindTab((GtkWidget*)hash);
        if (tab) {
            free(tab->rects);
        }
        g_hash_table_remove(qtcTabHashTable, hash);
    }
}

gboolean
qtcTabCurrentHoveredIndex(GtkWidget *widget)
{
    QtCTab *tab = qtcWidgetFindTab(widget);
    return tab ? tab->id : -1;
}

void
qtcTabUpdateRect(GtkWidget *widget, int tabIndex, int x, int y,
                 int width, int height)
{
    QtCTab *tab = qtcWidgetFindTab(widget);

    if (tab && tabIndex >= 0) {
        if (tabIndex >= tab->numRects) {
            tab->rects = (QtcRect*)realloc(tab->rects,
                                           sizeof(QtcRect) * (tabIndex + 8));
            for (int p = tab->numRects;p < tabIndex + 8;p++) {
                tab->rects[p].x = tab->rects[p].y = 0;
                tab->rects[p].width = tab->rects[p].height = -1;
            }
            tab->numRects = tabIndex + 8;
        }
        tab->rects[tabIndex].x = x;
        tab->rects[tabIndex].y = y;
        tab->rects[tabIndex].width = width;
        tab->rects[tabIndex].height = height;
    }
}

static void
qtcTabCleanup(GtkWidget *widget)
{
    if (widget) {
        QTC_DEF_WIDGET_PROPS(props, widget);
        qtcDisconnectFromProp(props, tabDestroy);
        qtcDisconnectFromProp(props, tabUnrealize);
        qtcDisconnectFromProp(props, tabStyleSet);
        qtcDisconnectFromProp(props, tabMotion);
        qtcDisconnectFromProp(props, tabLeave);
        qtcDisconnectFromProp(props, tabPageAdded);
        qtcWidgetProps(props)->tabHacked = true;
        qtcTabRemoveHash(widget);
    }
}

static gboolean
qtcTabStyleSet(GtkWidget *widget, GtkStyle *prev_style, void *data)
{
    QTC_UNUSED(prev_style);
    QTC_UNUSED(data);
    qtcTabCleanup(widget);
    return false;
}

static gboolean
qtcTabDestroy(GtkWidget *widget, GdkEvent *event, void *data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(data);
    qtcTabCleanup(widget);
    return false;
}

static void
qtcSetHoveredTab(QtCTab *tab, GtkWidget *widget, int index)
{
    if (tab->id != index) {
        QtcRect updateRect = {0, 0, -1, -1};
        tab->id = index;
        for (int p = 0;p < tab->numRects;p++) {
            qtcRectUnion(&tab->rects[p], &updateRect, &updateRect);
        }
        gtk_widget_queue_draw_area(widget, updateRect.x - 4, updateRect.y - 4,
                                   updateRect.width + 8, updateRect.height + 8);
    }
}

static gboolean
qtcTabMotion(GtkWidget *widget, GdkEventMotion *event, void *data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(data);
    QtCTab *tab = qtcWidgetFindTab(widget);
    if (tab) {
        int px;
        int py;
        gdk_window_get_pointer(gtk_widget_get_window(widget), &px, &py, NULL);

        for (int t = 0;t < tab->numRects;t++) {
            if (tab->rects[t].x <= px && tab->rects[t].y <= py &&
                tab->rects[t].x + tab->rects[t].width > px &&
                tab->rects[t].y + tab->rects[t].height > py) {
                qtcSetHoveredTab(tab, widget, t);
                return false;
            }
        }
        qtcSetHoveredTab(tab, widget, -1);
    }
    return false;
}

static gboolean
qtcTabLeave(GtkWidget *widget, GdkEventCrossing *event, void *data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(data);
    QtCTab *prevTab = qtcWidgetFindTab(widget);

    if (prevTab && prevTab->id >= 0) {
        prevTab->id = -1;
        gtk_widget_queue_draw(widget);
    }
    return false;
}

static void
qtcTabUnRegisterChild(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && qtcWidgetProps(props)->tabChildHacked) {
        qtcDisconnectFromProp(props, tabChildDestroy);
        qtcDisconnectFromProp(props, tabChildStyleSet);
        qtcDisconnectFromProp(props, tabChildEnter);
        qtcDisconnectFromProp(props, tabChildLeave);
        if (GTK_IS_CONTAINER(widget)) {
            qtcDisconnectFromProp(props, tabChildAdd);
        }
        qtcWidgetProps(props)->tabChildHacked = false;
    }
}

static void qtcTabUpdateChildren(GtkWidget *widget);

static gboolean
qtcTabChildMotion(GtkWidget *widget, GdkEventMotion *event, void *user_data)
{
    qtcTabMotion((GtkWidget*)user_data, event, widget);
    return false;
}

static gboolean
qtcTabChildDestroy(GtkWidget *widget, GdkEventCrossing *event, void *data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(data);
    qtcTabUnRegisterChild(widget);
    return false;
}

static gboolean
qtcTabChildStyleSet(GtkWidget *widget, GdkEventCrossing *event, void *data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(data);
    qtcTabUnRegisterChild(widget);
    return false;
}

static gboolean
qtcTabChildAdd(GtkWidget *widget, GdkEventCrossing *event, void *data)
{
    QTC_UNUSED(widget);
    QTC_UNUSED(event);
    qtcTabUpdateChildren((GtkWidget*)data);
    return false;
}

static void
qtcTabRegisterChild(GtkWidget *notebook, GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->tabChildHacked) {
        qtcWidgetProps(props)->tabChildHacked = true;
        qtcConnectToProp(props, tabChildDestroy, "destroy",
                         qtcTabChildDestroy, notebook);
        qtcConnectToProp(props, tabChildStyleSet, "style-set",
                         qtcTabChildStyleSet, notebook);
        qtcConnectToProp(props, tabChildEnter, "enter-notify-event",
                         qtcTabChildMotion, notebook);
        qtcConnectToProp(props, tabChildLeave, "leave-notify-event",
                         qtcTabChildMotion, notebook);
        if (GTK_IS_CONTAINER(widget)) {
            qtcConnectToProp(props, tabChildAdd, "add",
                             qtcTabChildAdd, notebook);
            GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
            for (GList *child = children;child;child = g_list_next(child)) {
                qtcTabRegisterChild(notebook, GTK_WIDGET(child->data));
            }
            if (children) {
                g_list_free(children);
            }
        }
    }
}

static void
qtcTabUpdateChildren(GtkWidget *widget)
{
    if (widget && GTK_IS_NOTEBOOK(widget)) {
        GtkNotebook *notebook = GTK_NOTEBOOK(widget);
        int numPages = gtk_notebook_get_n_pages(notebook);
        for (int i = 0;i < numPages;i++) {
            qtcTabRegisterChild(
                widget, gtk_notebook_get_tab_label(
                    notebook, gtk_notebook_get_nth_page(notebook, i)));
        }
    }
}

static gboolean
qtcTabPageAdded(GtkWidget *widget, GdkEventCrossing *event, void *data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(data);
    qtcTabUpdateChildren(widget);
    return false;
}

void
qtcTabSetup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->tabHacked) {
        qtcWidgetProps(props)->tabHacked = true;
        qtcTabLookupHash(widget, true);
        qtcConnectToProp(props, tabDestroy, "destroy-event",
                         qtcTabDestroy, NULL);
        qtcConnectToProp(props, tabUnrealize, "unrealize", qtcTabDestroy, NULL);
        qtcConnectToProp(props, tabStyleSet, "style-set", qtcTabStyleSet, NULL);
        qtcConnectToProp(props, tabMotion, "motion-notify-event",
                         qtcTabMotion, NULL);
        qtcConnectToProp(props, tabLeave, "leave-notify-event",
                         qtcTabLeave, NULL);
        qtcConnectToProp(props, tabPageAdded, "page-added",
                         qtcTabPageAdded, NULL);
        qtcTabUpdateChildren(widget);
    }
}

gboolean
qtcTabIsLabel(GtkNotebook *notebook, GtkWidget *widget)
{
    int numPages = gtk_notebook_get_n_pages(notebook);
    for (int i = 0;i < numPages;++i) {
        if (gtk_notebook_get_tab_label(
                notebook, gtk_notebook_get_nth_page(notebook, i)) == widget) {
            return true;
        }
    }
    return false;
}

QtcRect
qtcTabGetTabbarRect(GtkNotebook *notebook)
{
    QtcRect rect = {0, 0, -1, -1};
    QtcRect empty = rect;
    QtcRect pageAllocation;
    unsigned int borderWidth;
    int pageIndex;
    GtkWidget *page;
    GList *children = NULL;
    // check tab visibility
    if (!(gtk_notebook_get_show_tabs(notebook) &&
          (children = gtk_container_get_children(GTK_CONTAINER(notebook))))) {
        return empty;
    }
    g_list_free(children);
    // get full rect
    rect = qtcWidgetGetAllocation(GTK_WIDGET(notebook));

    // adjust to account for borderwidth
    borderWidth = gtk_container_get_border_width(GTK_CONTAINER(notebook));

    rect.x += borderWidth;
    rect.y += borderWidth;
    rect.height -= 2 * borderWidth;
    rect.width -= 2 * borderWidth;

    // get current page
    pageIndex = gtk_notebook_get_current_page(notebook);

    if (!(pageIndex >= 0 && pageIndex < gtk_notebook_get_n_pages(notebook))) {
        return empty;
    }
    page = gtk_notebook_get_nth_page(notebook, pageIndex);
    if (!page) {
        return empty;
    }

    // removes page allocated size from rect, based on tabwidget orientation
    pageAllocation = qtcWidgetGetAllocation(page);
    switch (gtk_notebook_get_tab_pos(notebook)) {
    case GTK_POS_BOTTOM:
        rect.y += pageAllocation.height;
        rect.height -= pageAllocation.height;
        break;
    case GTK_POS_TOP:
        rect.height -= pageAllocation.height;
        break;
    case GTK_POS_RIGHT:
        rect.x += pageAllocation.width;
        rect.width -= pageAllocation.width;
        break;
    case GTK_POS_LEFT:
        rect.width -= pageAllocation.width;
        break;
    }
    return rect;
}

gboolean
qtcTabHasVisibleArrows(GtkNotebook *notebook)
{
    if (gtk_notebook_get_show_tabs(notebook)) {
        int numPages = gtk_notebook_get_n_pages(notebook);
        for (int i = 0;i < numPages;i++) {
            GtkWidget *label = gtk_notebook_get_tab_label(
                notebook, gtk_notebook_get_nth_page(notebook, i));
#if GTK_CHECK_VERSION(2, 20, 0)
            if (label && !gtk_widget_get_mapped(label)) {
                return true;
            }
#else
            if (label && !GTK_WIDGET_MAPPED(label)) {
                return true;
            }
#endif
        }
    }
    return false;
}
